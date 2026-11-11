// Host check for afc.h: decodes the AFC sample written by make_stream_samples.py
// and compares it with ffmpeg's decode of the same stream. The script runs it.
//   afc_check <afc.bin> <ffmpeg s16le pcm>
#include "afc.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    std::ifstream a(argv[1], std::ios::binary), r(argv[2], std::ios::binary);
    std::vector<std::uint8_t> file{std::istreambuf_iterator<char>(a), {}}, ref{std::istreambuf_iterator<char>(r), {}};
    std::uint32_t head[5];
    std::memcpy(head, file.data(), sizeof head);
    const int channels = int(head[2]);
    const std::uint8_t* size_table = file.data() + sizeof head;
    const std::uint8_t* p = size_table + head[4] * 2;
    afc::History hist[2];
    std::vector<std::int16_t> pcm, buf(4096 * channels);
    for (std::uint32_t i = 0; i < head[4]; ++i) {
        std::uint16_t n;
        std::memcpy(&n, size_table + i * 2, 2);
        const int frames = afc::decode(hist, p, n, channels, buf.data());
        pcm.insert(pcm.end(), buf.begin(), buf.begin() + frames * channels);
        p += n;
    }
    const std::size_t ref_samples = ref.size() / 2;
    std::size_t diff = 0, common = std::min(pcm.size(), ref_samples);
    for (std::size_t i = 0; i < common; ++i) diff += pcm[i] != std::int16_t(ref[2 * i] | ref[2 * i + 1] << 8);
    std::printf("afc_check: %zu samples decoded, ffmpeg %zu, %zu differ\n", pcm.size(), ref_samples, diff);
    return diff == 0 && pcm.size() == ref_samples ? 0 : 1;
}
