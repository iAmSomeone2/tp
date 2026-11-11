// Vita storage and decompression benchmark for the asset-bake pack design
// (docs/port/bake.md, "Pack files and the index"). Results go to the screen and
// to ux0:data/nfbench/results.txt. Corpus: tools/vita-bench/make_corpus.py.

#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/power.h>

#include <lz4.h>
#include <lz4hc.h>
#include <zstd.h>

#include "debugScreen.h"

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <random>
#include <string>
#include <vector>

extern "C" {
// Peak use is about 50 MiB (corpus, output and compressed copies). A request
// larger than the app's memory budget leaves newlib with no heap at all.
unsigned int _newlib_heap_size_user = 96 * 1024 * 1024;
}

namespace {

constexpr const char* kOutDir = "ux0:data/nfbench";
constexpr const char* kRoots[] = {"app0:corpus", "ux0:data/nfbench/corpus", "uma0:data/nfbench/corpus",
                                  "imc0:data/nfbench/corpus"};

FILE* g_log = nullptr;

void out(const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    // debugScreen wraps to the top instead of scrolling, so keep the last
    // screenful of lines and redraw them.
    static std::deque<std::string> lines(1);
    const std::size_t rows = 544 / psvDebugScreenGetFont()->size_h - 1;
    for (const char* c = buf; *c; ++c) {
        if (*c == '\n')
            lines.emplace_back();
        else
            lines.back() += *c;
    }
    while (lines.size() > rows) lines.pop_front();
    std::string screen = "\e[H\e[2J";
    for (std::size_t i = 0; i < lines.size(); ++i) screen += lines[i] + (i + 1 < lines.size() ? "\n" : "");
    psvDebugScreenPuts(screen.c_str());
    if (g_log) {
        std::fputs(buf, g_log);
        std::fflush(g_log);
    }
}

std::uint64_t now_us() { return sceKernelGetProcessTimeWide(); }

struct Stats {
    std::vector<std::uint64_t> v;
    void add(std::uint64_t us) { v.push_back(us); }
    std::uint64_t total() const {
        std::uint64_t t = 0;
        for (auto x : v) t += x;
        return t;
    }
    void print(const char* label) {
        if (v.empty()) return;
        auto s = v;
        std::sort(s.begin(), s.end());
        out("  %-26s n=%3u min %8.2f med %8.2f p90 %8.2f max %8.2f ms\n", label, unsigned(s.size()), s.front() / 1e3,
            s[s.size() / 2] / 1e3, s[s.size() * 9 / 10] / 1e3, s.back() / 1e3);
    }
};

double mibps(std::uint64_t bytes, std::uint64_t us) { return us ? bytes / (1024.0 * 1024.0) / (us / 1e6) : 0; }

struct Entry {
    std::string name;
    std::uint32_t offset, size;
};

std::vector<Entry> read_index(const std::string& root) {
    std::vector<Entry> idx;
    FILE* f = std::fopen((root + "/pack.idx").c_str(), "r");
    if (!f) return idx;
    char name[128];
    unsigned off, size;
    while (std::fscanf(f, "%127s %u %u", name, &off, &size) == 3) idx.push_back({name, off, size});
    std::fclose(f);
    return idx;
}

// Open/close cost split, then the loose-file pattern (open, read whole, close per entry).
void bench_loose(const std::string& root, const std::vector<Entry>& idx, std::vector<std::uint8_t>& buf) {
    for (int pass = 1; pass <= 2; ++pass) {
        Stats open_s, read_s, close_s, stat_s;
        std::uint64_t bytes = 0;
        for (const auto& e : idx) {
            std::string path = root + "/files/" + e.name + ".bin";
            SceIoStat st;
            auto t0 = now_us();
            sceIoGetstat(path.c_str(), &st);
            auto t1 = now_us();
            SceUID fd = sceIoOpen(path.c_str(), SCE_O_RDONLY, 0);
            auto t2 = now_us();
            if (fd < 0) {
                out("  open failed %s: 0x%08X\n", path.c_str(), fd);
                return;
            }
            int n = sceIoRead(fd, buf.data(), e.size);
            auto t3 = now_us();
            sceIoClose(fd);
            auto t4 = now_us();
            stat_s.add(t1 - t0);
            open_s.add(t2 - t1);
            read_s.add(t3 - t2);
            close_s.add(t4 - t3);
            bytes += n > 0 ? n : 0;
        }
        out(" loose files, pass %d:\n", pass);
        stat_s.print("getstat");
        open_s.print("open (read-only)");
        read_s.print("read whole file");
        close_s.print("close");
        std::uint64_t total = stat_s.total() + open_s.total() + read_s.total() + close_s.total();
        out("  all %u entries: %.1f ms, %.2f MiB/s effective\n", unsigned(idx.size()), total / 1e3, mibps(bytes, total));
    }
}

// The pack pattern: one handle, offset reads.
void bench_pack(const std::string& root, const std::vector<Entry>& idx, std::vector<std::uint8_t>& buf) {
    std::string path = root + "/pack.bin";
    auto t0 = now_us();
    SceUID fd = sceIoOpen(path.c_str(), SCE_O_RDONLY, 0);
    auto t1 = now_us();
    if (fd < 0) {
        out("  open failed %s: 0x%08X\n", path.c_str(), fd);
        return;
    }
    out(" pack: open %.2f ms\n", (t1 - t0) / 1e3);
    std::uint64_t pack_size = idx.back().offset + idx.back().size;

    for (int pass = 1; pass <= 2; ++pass) {
        Stats s;
        std::uint64_t bytes = 0;
        for (const auto& e : idx) {
            auto a = now_us();
            int n = sceIoPread(fd, buf.data(), e.size, e.offset);
            s.add(now_us() - a);
            bytes += n > 0 ? n : 0;
        }
        out(" pack entries via pread, pass %d:\n", pass);
        s.print("pread whole entry");
        out("  all %u entries: %.1f ms, %.2f MiB/s\n", unsigned(idx.size()), s.total() / 1e3, mibps(bytes, s.total()));
    }

    for (std::uint32_t chunk : {64u << 10, 256u << 10, 1u << 20}) {
        auto a = now_us();
        std::uint64_t bytes = 0;
        for (std::uint64_t off = 0; off < pack_size; off += chunk) {
            int n = sceIoPread(fd, buf.data(), std::min<std::uint64_t>(chunk, pack_size - off), off);
            if (n <= 0) break;
            bytes += n;
        }
        auto us = now_us() - a;
        out("  sequential %4u KiB chunks: %.2f MiB/s\n", chunk >> 10, mibps(bytes, us));
    }

    std::mt19937 rng(1234);
    for (std::uint32_t size : {4u << 10, 64u << 10}) {
        Stats s;
        std::uniform_int_distribution<std::uint64_t> pick(0, (pack_size - size) / size);
        for (int i = 0; i < 100; ++i) {
            auto a = now_us();
            sceIoPread(fd, buf.data(), size, pick(rng) * size);
            s.add(now_us() - a);
        }
        char label[48];
        std::snprintf(label, sizeof label, "random pread %u KiB", size >> 10);
        s.print(label);
    }
    auto a = now_us();
    sceIoClose(fd);
    out("  pack close %.2f ms\n", (now_us() - a) / 1e3);
}

void bench_writes(std::vector<std::uint8_t>& buf) {
    std::string dir = std::string(kOutDir) + "/tmp";
    sceIoMkdir(dir.c_str(), 0777);
    for (int sync = 0; sync <= 1; ++sync) {
        Stats open_s, write_s, sync_s, close_s;
        for (int i = 0; i < 10; ++i) {
            std::string path = dir + "/w" + std::to_string(i) + ".bin";
            auto t0 = now_us();
            SceUID fd = sceIoOpen(path.c_str(), SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
            auto t1 = now_us();
            if (fd < 0) {
                out("  write open failed: 0x%08X\n", fd);
                return;
            }
            sceIoWrite(fd, buf.data(), 64 << 10);
            auto t2 = now_us();
            if (sync) sceIoSyncByFd(fd, 0);
            auto t3 = now_us();
            sceIoClose(fd);
            auto t4 = now_us();
            open_s.add(t1 - t0);
            write_s.add(t2 - t1);
            sync_s.add(t3 - t2);
            close_s.add(t4 - t3);
        }
        out(" writes, 64 KiB per file, %s:\n", sync ? "with sceIoSyncByFd" : "no sync");
        open_s.print("open (create/truncate)");
        write_s.print("write");
        if (sync) sync_s.print("sync");
        close_s.print("close");
    }
    for (int i = 0; i < 10; ++i) sceIoRemove((dir + "/w" + std::to_string(i) + ".bin").c_str());
    sceIoRmdir(dir.c_str());
}

struct Codec {
    const char* name;
    int level;
};

void bench_codecs(const std::string& root, const std::vector<Entry>& idx, double pack_mibps) {
    std::vector<std::uint8_t> raw(idx.back().offset + idx.back().size);
    SceUID fd = sceIoOpen((root + "/pack.bin").c_str(), SCE_O_RDONLY, 0);
    sceIoPread(fd, raw.data(), raw.size(), 0);
    sceIoClose(fd);

    std::vector<std::uint8_t> dst(raw.size());
    auto best_of_3 = [](auto&& fn) {
        std::uint64_t best = UINT64_MAX;
        for (int i = 0; i < 3; ++i) {
            auto a = now_us();
            fn();
            best = std::min(best, now_us() - a);
        }
        return best;
    };

    std::uint64_t memcpy_us = best_of_3([&] { std::memcpy(dst.data(), raw.data(), raw.size()); });
    out(" decompression (one core, best of 3, MiB/s of output), corpus %.2f MiB:\n", raw.size() / 1048576.0);
    out("  %-12s ratio %5.2f  decode %8.1f\n", "memcpy", 1.0, mibps(raw.size(), memcpy_us));

    const Codec codecs[] = {{"lz4", 1}, {"lz4hc-9", 9}, {"lz4hc-12", 12}, {"zstd-3", 3}, {"zstd-9", 9}};
    ZSTD_DCtx* dctx = ZSTD_createDCtx();
    for (const auto& c : codecs) {
        bool is_zstd = c.name[0] == 'z';
        bool is_hc = std::strncmp(c.name, "lz4hc", 5) == 0;
        std::vector<std::vector<std::uint8_t>> comp(idx.size());
        std::uint64_t stored = 0;
        auto ca = now_us();
        for (size_t i = 0; i < idx.size(); ++i) {
            const auto* src = raw.data() + idx[i].offset;
            int n = idx[i].size;
            if (is_zstd) {
                comp[i].resize(ZSTD_compressBound(n));
                comp[i].resize(ZSTD_compress(comp[i].data(), comp[i].size(), src, n, c.level));
            } else {
                comp[i].resize(LZ4_compressBound(n));
                int m = is_hc ? LZ4_compress_HC((const char*)src, (char*)comp[i].data(), n, comp[i].size(), c.level)
                              : LZ4_compress_default((const char*)src, (char*)comp[i].data(), n, comp[i].size());
                comp[i].resize(m);
            }
            stored += comp[i].size();
        }
        auto comp_us = now_us() - ca;

        std::uint64_t us = best_of_3([&] {
            for (size_t i = 0; i < idx.size(); ++i) {
                auto* d = dst.data() + idx[i].offset;
                if (is_zstd)
                    ZSTD_decompressDCtx(dctx, d, idx[i].size, comp[i].data(), comp[i].size());
                else
                    LZ4_decompress_safe((const char*)comp[i].data(), (char*)d, comp[i].size(), idx[i].size);
            }
        });
        bool ok = std::memcmp(dst.data(), raw.data(), raw.size()) == 0;
        double ratio = double(raw.size()) / stored;
        // Estimated load time for the corpus: read the stored bytes at the measured
        // pack throughput, then decompress.
        double load_ms = pack_mibps > 0 ? stored / 1048576.0 / pack_mibps * 1e3 + us / 1e3 : 0;
        out("  %-12s ratio %5.2f  decode %8.1f  (compress %.1f s)  est. load %.0f ms%s\n", c.name, ratio,
            mibps(raw.size(), us), comp_us / 1e6, load_ms, ok ? "" : "  MISMATCH");
        std::memset(dst.data(), 0, dst.size());
    }
    ZSTD_freeDCtx(dctx);
    if (pack_mibps > 0)
        out("  %-12s est. load %.0f ms (uncompressed read)\n", "none", raw.size() / 1048576.0 / pack_mibps * 1e3);
}

// Uncached-ish throughput for the load estimate: first pass over the pack in 1 MiB reads.
double measure_pack_mibps(const std::string& root, std::size_t size, std::vector<std::uint8_t>& buf) {
    SceUID fd = sceIoOpen((root + "/pack.bin").c_str(), SCE_O_RDONLY, 0);
    if (fd < 0) return 0;
    auto a = now_us();
    std::uint64_t bytes = 0;
    for (std::uint64_t off = 0; off < size; off += 1 << 20) {
        int n = sceIoPread(fd, buf.data(), std::min<std::uint64_t>(1 << 20, size - off), off);
        if (n <= 0) break;
        bytes += n;
    }
    auto us = now_us() - a;
    sceIoClose(fd);
    return mibps(bytes, us);
}

void wait_for_exit() {
    out("\nDone. Results: %s/results.txt. Press START to exit.\n", kOutDir);
    SceCtrlData pad;
    do {
        sceCtrlPeekBufferPositive(0, &pad, 1);
    } while (!(pad.buttons & SCE_CTRL_START));
}

}  // namespace

int main() {
    psvDebugScreenInit();
    scePowerSetArmClockFrequency(444);
    sceIoMkdir(kOutDir, 0777);
    g_log = std::fopen((std::string(kOutDir) + "/results.txt").c_str(), "w");

    out("nfbench: ARM %d MHz, bus %d MHz\n", scePowerGetArmClockFrequency(), scePowerGetBusClockFrequency());
    SceKernelFreeMemorySizeInfo mem{};
    mem.size = sizeof mem;
    sceKernelGetFreeMemorySize(&mem);
    out("memory: newlib heap %u MiB; free after it: user %.1f MiB, CDRAM %.1f MiB, phycont %.1f MiB\n",
        _newlib_heap_size_user >> 20, mem.size_user / 1048576.0, mem.size_cdram / 1048576.0,
        mem.size_phycont / 1048576.0);

    std::vector<std::uint8_t> buf(8 << 20);
    std::string codec_root;
    double codec_mibps = 0;
    for (const char* root : kRoots) {
        auto idx = read_index(root);
        if (idx.empty()) {
            out("\n[%s] no corpus, skipped\n", root);
            continue;
        }
        std::size_t max_entry = 0;
        for (const auto& e : idx) max_entry = std::max<std::size_t>(max_entry, e.size);
        if (buf.size() < max_entry) buf.resize(max_entry);
        out("\n[%s] %u entries\n", root, unsigned(idx.size()));
        // Before anything else reads the pack, so the OS cache is as cold as it gets.
        double mibps_cold = measure_pack_mibps(root, idx.back().offset + idx.back().size, buf);
        out(" first sequential read of pack: %.2f MiB/s\n", mibps_cold);
        bench_loose(root, idx, buf);
        bench_pack(root, idx, buf);
        if (codec_root.empty()) {
            codec_root = root;
            codec_mibps = mibps_cold;
        }
    }

    out("\n[writes to %s]\n", kOutDir);
    bench_writes(buf);

    if (!codec_root.empty()) {
        out("\n[codecs, corpus from %s]\n", codec_root.c_str());
        bench_codecs(codec_root, read_index(codec_root), codec_mibps);
    }

    if (g_log) std::fclose(g_log);
    g_log = nullptr;
    wait_for_exit();
    sceKernelExitProcess(0);
    return 0;
}
