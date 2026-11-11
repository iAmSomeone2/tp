// Stream decode cost on the Vita. Plays app0:sample.bin (make_stream_samples.py)
// through SDL3's audio stream, decoding on SDL's audio thread as the port's mixer
// would, and times every decode call. Then decodes the whole track again flat out,
// without audio, as a cross-check. Results go to the screen and to
// ux0:data/nfbench/<variant>.txt; the app exits by itself.
//
// Built three times (CMakeLists.txt): libopus in float (opus_decode_float, F32 to
// SDL), libopus in fixed point (opus_decode, S16), and AFC ADPCM as on the disc
// (afc.h, S16), for comparison.

#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>

#include <SDL3/SDL.h>

#if NFBENCH_AFC
#include "afc.h"
#else
#include <opus.h>
#endif

#include "debugScreen.h"

#include <algorithm>
#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

constexpr int kMaxFrames = 5760;  // per packet: Opus's 120 ms maximum; AFC packets hold 960

// One decoder per variant: decode() returns the frames written, or < 0 on error.
#if NFBENCH_AFC
using Sample = std::int16_t;
constexpr SDL_AudioFormat kFormat = SDL_AUDIO_S16;
constexpr const char* kVariant = "afc";
constexpr const char kMagic[] = "NFAF";
struct Decoder {
    int channels;
    afc::History hist[2];
    Decoder(int, int ch) : channels(ch) {}
    const char* version() const { return "AFC ADPCM"; }
    int decode(const unsigned char* p, int n, Sample* out) { return afc::decode(hist, p, n, channels, out); }
    int frames(const unsigned char*, int n) const { return n / channels / 9 * 16; }
    void reset() { hist[0] = hist[1] = {}; }
};
#else
#if NFOPUS_FIXED
using Sample = opus_int16;
constexpr SDL_AudioFormat kFormat = SDL_AUDIO_S16;
constexpr const char* kVariant = "opus-fixed";
#else
using Sample = float;
constexpr SDL_AudioFormat kFormat = SDL_AUDIO_F32;
constexpr const char* kVariant = "opus-float";
#endif
constexpr const char kMagic[] = "NFOP";
struct Decoder {
    OpusDecoder* dec;
    int rate;
    Decoder(int r, int ch) : rate(r) {
        int err = 0;
        dec = opus_decoder_create(r, ch, &err);
        if (err != OPUS_OK) dec = nullptr;
    }
    ~Decoder() { opus_decoder_destroy(dec); }
    const char* version() const { return opus_get_version_string(); }
#if NFOPUS_FIXED
    int decode(const unsigned char* p, int n, Sample* out) { return opus_decode(dec, p, n, out, kMaxFrames, 0); }
#else
    int decode(const unsigned char* p, int n, Sample* out) { return opus_decode_float(dec, p, n, out, kMaxFrames, 0); }
#endif
    int frames(const unsigned char* p, int n) const { return opus_packet_get_nb_samples(p, n, rate); }
    void reset() { opus_decoder_ctl(dec, OPUS_RESET_STATE); }
};
#endif

FILE* g_log = nullptr;

void out(const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    psvDebugScreenPuts(buf);
    if (g_log) std::fputs(buf, g_log);
}

[[noreturn]] void fail(const char* what) {
    out("FAILED: %s\n", what);
    if (g_log) std::fclose(g_log);
    sceKernelDelayThread(10'000'000);
    sceKernelExitProcess(1);
    for (;;) {}
}

struct Track {
    std::vector<unsigned char> file;
    std::uint32_t rate = 0, channels = 0, pre_skip = 0;
    std::vector<const unsigned char*> packet;
    std::vector<std::uint16_t> size;
};

Track load(const char* path) {
    Track t;
    FILE* f = std::fopen(path, "rb");
    if (!f) fail("open app0:sample.bin (configure with -DNFBENCH_SAMPLES)");
    std::fseek(f, 0, SEEK_END);
    t.file.resize(std::ftell(f));
    std::fseek(f, 0, SEEK_SET);
    std::fread(t.file.data(), 1, t.file.size(), f);
    std::fclose(f);

    std::uint32_t head[5];
    std::memcpy(head, t.file.data(), sizeof head);
    if (std::memcmp(head, kMagic, 4) != 0) fail("bad sample magic");
    t.rate = head[1], t.channels = head[2], t.pre_skip = head[3];
    t.size.resize(head[4]);
    std::memcpy(t.size.data(), t.file.data() + sizeof head, t.size.size() * 2);
    const unsigned char* p = t.file.data() + sizeof head + t.size.size() * 2;
    for (std::uint16_t n : t.size) t.packet.push_back(p), p += n;
    return t;
}

// Shared between the audio callback and the main thread.
struct Play {
    const Track* track;
    Decoder* dec;
    std::size_t next = 0;
    std::uint32_t skip;
    std::vector<Sample> pcm;
    // Times in microseconds.
    std::uint64_t decode_total = 0, decode_max = 0, callback_max = 0;
    std::uint32_t callbacks = 0, errors = 0;
    std::atomic<bool> done{false};
};

void SDLCALL feed(void* user, SDL_AudioStream* stream, int additional, int) {
    auto& s = *static_cast<Play*>(user);
    const std::uint64_t cb_start = sceKernelGetProcessTimeWide();
    const int frame_bytes = int(s.track->channels * sizeof(Sample));
    while (additional > 0 && s.next < s.track->packet.size()) {
        const std::uint64_t t0 = sceKernelGetProcessTimeWide();
        int frames = s.dec->decode(s.track->packet[s.next], s.track->size[s.next], s.pcm.data());
        const std::uint64_t dt = sceKernelGetProcessTimeWide() - t0;
        ++s.next;
        if (frames < 0) { ++s.errors; continue; }
        s.decode_total += dt;
        s.decode_max = std::max(s.decode_max, dt);
        const std::uint32_t skip = std::min<std::uint32_t>(s.skip, frames);
        s.skip -= skip;
        const int bytes = (frames - int(skip)) * frame_bytes;
        SDL_PutAudioStreamData(stream, s.pcm.data() + skip * s.track->channels, bytes);
        additional -= bytes;
    }
    if (s.next == s.track->packet.size()) s.done = true;
    s.callback_max = std::max<std::uint64_t>(s.callback_max, sceKernelGetProcessTimeWide() - cb_start);
    ++s.callbacks;
}

}  // namespace

int main() {
    psvDebugScreenInit();
    scePowerSetArmClockFrequency(444);
    sceIoMkdir("ux0:data/nfbench", 0777);
    char path[64];
    std::snprintf(path, sizeof path, "ux0:data/nfbench/%s.txt", kVariant);
    g_log = std::fopen(path, "w");

    const Track track = load("app0:sample.bin");
    Decoder dec(int(track.rate), int(track.channels));
    out("streambench (%s, %s): ARM %d MHz\n", kVariant, dec.version(), scePowerGetArmClockFrequency());
    std::size_t total_frames = 0;
    for (std::size_t i = 0; i < track.packet.size(); ++i) total_frames += dec.frames(track.packet[i], track.size[i]);
    const double seconds = double(total_frames) / track.rate;
    const double packet_ms = 1000.0 * dec.frames(track.packet[0], track.size[0]) / track.rate;
    std::size_t bytes = 0;
    for (std::uint16_t n : track.size) bytes += n;
    out("sample: %u packets, %.1f s, %u ch, %u Hz, %.0f kbit/s\n", unsigned(track.packet.size()), seconds,
        track.channels, track.rate, bytes * 8 / seconds / 1000);

#if !NFBENCH_AFC
    if (!dec.dec) fail("opus_decoder_create");
#endif
    Play play{&track, &dec};
    play.skip = track.pre_skip;
    play.pcm.resize(kMaxFrames * track.channels);

    if (!SDL_Init(SDL_INIT_AUDIO)) fail(SDL_GetError());
    const SDL_AudioSpec spec{kFormat, int(track.channels), int(track.rate)};
    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, feed, &play);
    if (!stream) fail(SDL_GetError());
    SDL_AudioSpec dev{};
    int dev_frames = 0;
    SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream), &dev, &dev_frames);
    out("device: %s, %d ch, %d Hz, %d frames per buffer\n\nplaying...\n", SDL_GetAudioFormatName(dev.format),
        dev.channels, dev.freq, dev_frames);

    const std::uint64_t wall_start = sceKernelGetProcessTimeWide();
    SDL_ResumeAudioStreamDevice(stream);
    while (!play.done || SDL_GetAudioStreamQueued(stream) > 0) SDL_Delay(100);
    SDL_Delay(500);  // let the device drain its last buffer
    const double wall = (sceKernelGetProcessTimeWide() - wall_start) / 1e6;
    SDL_DestroyAudioStream(stream);
    SDL_Quit();

    const double n = double(track.packet.size());
    out("\nwhile playing (SDL audio thread), %u callbacks, %.1f s wall clock, %u decode errors:\n", play.callbacks, wall,
        play.errors);
    out("  decode total %.3f s = %.2f%% of one core\n", play.decode_total / 1e6, 100.0 * play.decode_total / 1e6 / seconds);
    out("  per %.1f ms packet: mean %.0f us, max %llu us\n", packet_ms, play.decode_total / n,
        (unsigned long long)play.decode_max);
    out("  longest callback %llu us\n", (unsigned long long)play.callback_max);

    // Flat out, same decoder settings, no audio thread in the way.
    dec.reset();
    std::uint64_t flat_max = 0;
    const std::uint64_t t0 = sceKernelGetProcessTimeWide();
    for (std::size_t i = 0; i < track.packet.size(); ++i) {
        const std::uint64_t p0 = sceKernelGetProcessTimeWide();
        dec.decode(track.packet[i], track.size[i], play.pcm.data());
        flat_max = std::max<std::uint64_t>(flat_max, sceKernelGetProcessTimeWide() - p0);
    }
    const double flat = (sceKernelGetProcessTimeWide() - t0) / 1e6;
    out("\nflat out (main thread): %.3f s = %.2f%% of one core, %.0fx real time\n", flat, 100.0 * flat / seconds,
        seconds / flat);
    out("  per packet: mean %.0f us, max %llu us\n", flat * 1e6 / n, (unsigned long long)flat_max);

    out("\nDone. Results: %s. Exiting in 10 s.\n", path);
    if (g_log) std::fclose(g_log);
    sceKernelDelayThread(10'000'000);
    sceKernelExitProcess(0);
    return 0;
}
