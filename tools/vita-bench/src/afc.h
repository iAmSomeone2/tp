// AFC ADPCM decoding, the codec of the game's .ast streams (JASAramStream format 0).
// A frame is 9 bytes per channel: a header byte (scale exponent << 4 | predictor
// index) and 16 four-bit samples, high nibble first. Coefficients as in ffmpeg's
// adpcm_afc decoder, which the host check (afc_check.cpp) compares against.
#pragma once

#include <algorithm>
#include <cstdint>

namespace afc {

inline constexpr std::int16_t kCoef[2][16] = {
    {0, 2048, 0, 1024, 4096, 3584, 3072, 4608, 4200, 4800, 5120, 2048, 1024, -1024, -1024, -2048},
    {0, 0, 2048, 1024, -2048, -1536, -1024, -2560, -2248, -2300, -3072, -2048, -1024, 1024, 0, 0},
};

struct History {
    int last = 0, penult = 0;
};

// One packet holds the same number of frames for each channel, channel after
// channel. Writes interleaved samples to out and returns the frames per channel.
inline int decode(History* hist, const std::uint8_t* p, int size, int channels, std::int16_t* out) {
    const int frames = size / channels / 9;
    for (int c = 0; c < channels; ++c) {
        History& h = hist[c];
        std::int16_t* o = out + c;
        for (int f = 0; f < frames; ++f, p += 9) {
            const int scale = 1 << (p[0] >> 4);
            const int c1 = kCoef[0][p[0] & 15], c2 = kCoef[1][p[0] & 15];
            for (int i = 0; i < 16; ++i) {
                const int nibble = std::int8_t(i & 1 ? p[1 + i / 2] << 4 : p[1 + i / 2]) >> 4;
                const int s = std::clamp(((h.last * c1 + h.penult * c2) >> 11) + nibble * scale, -32768, 32767);
                h.penult = h.last;
                h.last = s;
                *o = std::int16_t(s);
                o += channels;
            }
        }
    }
    return frames * 16;
}

}  // namespace afc
