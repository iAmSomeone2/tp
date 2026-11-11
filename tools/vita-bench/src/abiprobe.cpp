// ABI probe for the memory design (docs/port/memory-design.md, step 0).
//
// 1. thread_local: the current heap becomes a thread_local pointer. GCC for
//    arm-vita-eabi implements it with emutls (__emutls_get_address on every
//    access, backed by pthread keys), so it is checked on std::thread and on raw
//    sceKernelCreateThread threads, and its cost per access is timed.
// 2. Alignment: which loads fault at which address alignment, to choose the heaps'
//    minimum alignment (4 on the console). Hand-written instructions cover the
//    cases exactly; compiled functions show what GCC emits for 8-aligned types.
//
// A faulting load kills the app. Every case is logged to
// ux0:data/nfbench/abiprobe.txt before it runs, so a case that started but never
// finished is the one that faulted. Launching the app again marks it "FAULT" and
// carries on with the next case; repeat until the file ends with "done".

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>

#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

const char* const kPath = "ux0:data/nfbench/abiprobe.txt";
std::string g_log;  // the file's contents when the app started

// One line per call, through open/write/close, so it is on storage before a fault.
void line(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void line(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = std::vsnprintf(buf, sizeof buf - 1, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (n > int(sizeof buf - 2)) n = sizeof buf - 2;
    buf[n++] = '\n';
    SceUID fd = sceIoOpen(kPath, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
    if (fd < 0) return;
    sceIoWrite(fd, buf, n);
    sceIoClose(fd);
}

bool logged(const std::string& s) { return g_log.find(s + "\n") != std::string::npos; }

// Runs fn as case `name` unless an earlier launch already finished or faulted on it.
template <class Fn>
void run_case(const char* name, Fn fn) {
    std::string b = std::string("begin ") + name, e = std::string("end   ") + name,
                f = std::string("FAULT ") + name;
    if (logged(f) || g_log.find(e) != std::string::npos) return;
    if (logged(b)) {
        line("%s", f.c_str());
        return;
    }
    line("%s", b.c_str());
    std::string result = fn();
    line("%s  %s", e.c_str(), result.c_str());
}

// ---- 1. thread_local ----------------------------------------------------------

thread_local int* t_current = nullptr;  // stands in for JKRHeap::sCurrentHeap
int* volatile g_plain = nullptr;
std::atomic<int> g_bad{0};

int* tls_get() __attribute__((noinline));
int* tls_get() { return t_current; }

// Each thread claims its own slot, then checks it while the others write theirs.
void tls_worker(int* mine) {
    if (t_current != nullptr) g_bad++;  // a new thread must start with the initial value
    t_current = mine;
    for (int i = 0; i < 200000; i++) {
        if (tls_get() != mine) g_bad++;
        if ((i & 1023) == 0) std::this_thread::yield();
    }
}

int sce_entry(SceSize, void* argp) {
    tls_worker(*static_cast<int**>(argp));
    return 0;
}

std::string tls_std_thread() {
    int main_slot = 0, slots[4] = {};
    t_current = &main_slot;
    g_bad = 0;
    std::vector<std::thread> threads;
    for (int& s : slots) threads.emplace_back(tls_worker, &s);
    for (auto& t : threads) t.join();
    if (t_current != &main_slot) g_bad++;
    return g_bad ? "FAILED, " + std::to_string(g_bad.load()) + " mismatches" : "ok";
}

std::string tls_sce_thread() {
    int main_slot = 0, slots[4] = {};
    int* args[4];
    t_current = &main_slot;
    g_bad = 0;
    SceUID ids[4];
    for (int i = 0; i < 4; i++) {
        args[i] = &slots[i];
        ids[i] = sceKernelCreateThread("tlsprobe", sce_entry, 0x10000100, 0x10000, 0, 0, nullptr);
        if (ids[i] < 0) return "FAILED, sceKernelCreateThread";
        int* a = args[i];
        sceKernelStartThread(ids[i], sizeof a, &a);
    }
    for (SceUID id : ids) {
        int stat;
        sceKernelWaitThreadEnd(id, &stat, nullptr);
        sceKernelDeleteThread(id);
    }
    if (t_current != &main_slot) g_bad++;
    return g_bad ? "FAILED, " + std::to_string(g_bad.load()) + " mismatches" : "ok";
}

std::string tls_cost() {
    constexpr int kN = 10'000'000;
    int x = 0;
    t_current = &x;
    uintptr_t sum = 0;
    SceUInt64 t0 = sceKernelGetProcessTimeWide();
    for (int i = 0; i < kN; i++) sum += uintptr_t(tls_get());
    SceUInt64 t1 = sceKernelGetProcessTimeWide();
    g_plain = &x;
    for (int i = 0; i < kN; i++) sum += uintptr_t(g_plain);
    SceUInt64 t2 = sceKernelGetProcessTimeWide();
    char buf[96];
    std::snprintf(buf, sizeof buf, "thread_local %.1f ns, plain global %.1f ns per read (%u)",
                  (t1 - t0) * 1000.0 / kN, (t2 - t1) * 1000.0 / kN, unsigned(sum & 1));
    return buf;
}

// ---- 2. alignment -------------------------------------------------------------

alignas(64) unsigned char g_buf[256];

// The address `off` bytes past a 32-byte boundary.
void* at(unsigned off) { return g_buf + 32 + off; }

std::string ok() { return "ok"; }

std::string ldr_at(unsigned off) {
    uint32_t v;
    asm volatile("ldr %0, [%1]" : "=r"(v) : "r"(at(off)) : "memory");
    return ok();
}
std::string ldrh_at(unsigned off) {
    uint32_t v;
    asm volatile("ldrh %0, [%1]" : "=r"(v) : "r"(at(off)) : "memory");
    return ok();
}
std::string ldrd_at(unsigned off) {
    register uint32_t lo asm("r2"), hi asm("r3");
    asm volatile("ldrd %0, %1, [%2]" : "=r"(lo), "=r"(hi) : "r"(at(off)) : "memory");
    return ok();
}
std::string ldm_at(unsigned off) {
    asm volatile("ldm %0, {r2, r3}" : : "r"(at(off)) : "r2", "r3", "memory");
    return ok();
}
std::string vldr_at(unsigned off) {
    asm volatile("vldr d0, [%0]" : : "r"(at(off)) : "d0", "memory");
    return ok();
}
std::string vld1_none_at(unsigned off) {
    asm volatile("vld1.8 {d0-d1}, [%0]" : : "r"(at(off)) : "d0", "d1", "memory");
    return ok();
}
std::string vld1_64_at(unsigned off) {
    asm volatile("vld1.64 {d0}, [%0:64]" : : "r"(at(off)) : "d0", "memory");
    return ok();
}
std::string vld1_128_at(unsigned off) {
    asm volatile("vld1.64 {d0-d1}, [%0:128]" : : "r"(at(off)) : "d0", "d1", "memory");
    return ok();
}

// What GCC emits for types whose alignof is 8, called through a pointer that is
// only 4-aligned, as a heap with a minimum of 4 would hand out. -O3 lets the
// vectoriser use NEON with alignment hints it derives from the type.
struct Pair {
    double a;
    uint64_t b;
};
static_assert(alignof(Pair) == 8);

__attribute__((noinline, optimize("O3"))) uint64_t sum_u64(const uint64_t* p, int n) {
    uint64_t s = 0;
    for (int i = 0; i < n; i++) s += p[i];
    return s;
}
__attribute__((noinline, optimize("O3"))) void copy_pairs(Pair* d, const Pair* s, int n) {
    for (int i = 0; i < n; i++) d[i] = s[i];
}
__attribute__((noinline, optimize("O3"))) double sum_pairs(const Pair* p, int n) {
    double s = 0;
    for (int i = 0; i < n; i++) s += p[i].a + double(p[i].b);
    return s;
}

std::string compiled_u64_at(unsigned off) {
    volatile uint64_t r = sum_u64(static_cast<const uint64_t*>(at(off)), 16);
    (void)r;
    return ok();
}
std::string compiled_copy_at(unsigned off) {
    copy_pairs(static_cast<Pair*>(at(off)), static_cast<const Pair*>(at(off + 96)), 4);
    return ok();
}
std::string compiled_double_at(unsigned off) {
    volatile double r = sum_pairs(static_cast<const Pair*>(at(off)), 4);
    (void)r;
    return ok();
}

std::string malloc_alignment() {
    // The lowest set bit over many block addresses is what malloc guarantees in practice.
    uintptr_t bits = 0;
    std::vector<void*> blocks;
    for (int i = 1; i <= 64; i++) {
        void* p = std::malloc(i * 3);
        blocks.push_back(p);
        bits |= uintptr_t(p);
    }
    for (int i = 1; i <= 64; i++) {
        auto p = std::make_unique<char[]>(i * 5);
        bits |= uintptr_t(p.get());
    }
    for (void* p : blocks) std::free(p);
    char buf[128];
    std::snprintf(buf, sizeof buf, "malloc/new give %u-byte alignment; max_align_t %u, default new %u",
                  unsigned(bits & -bits), unsigned(alignof(std::max_align_t)),
                  unsigned(__STDCPP_DEFAULT_NEW_ALIGNMENT__));
    return buf;
}

}  // namespace

int main() {
    sceIoMkdir("ux0:data/nfbench", 0777);
    if (FILE* f = std::fopen(kPath, "r")) {
        char buf[512];
        while (std::fgets(buf, sizeof buf, f)) g_log += buf;
        std::fclose(f);
    }
    if (logged("done")) {  // a finished run: start over
        sceIoRemove(kPath);
        g_log.clear();
    }
    if (g_log.empty()) line("abiprobe; launch again after a crash until the last line is \"done\"");

    run_case("tls std::thread", tls_std_thread);
    run_case("tls sceKernelCreateThread", tls_sce_thread);
    run_case("tls cost", tls_cost);
    run_case("malloc alignment", malloc_alignment);

    // Safest first; each instruction at offsets 4 and 8 (and 1 for the scalar loads).
    struct Load {
        const char* name;
        std::string (*fn)(unsigned);
        unsigned offs[3];
    } loads[] = {
        {"ldr", ldr_at, {1, 2, 4}},
        {"ldrh", ldrh_at, {1, 0, 0}},
        {"ldm", ldm_at, {4, 0, 0}},
        {"ldrd", ldrd_at, {4, 0, 0}},
        {"vldr", vldr_at, {4, 0, 0}},
        {"vld1 no hint", vld1_none_at, {1, 4, 0}},
        {"vld1 :64", vld1_64_at, {4, 0, 0}},
        {"vld1 :128", vld1_128_at, {4, 8, 0}},
        {"gcc u64 sum", compiled_u64_at, {4, 0, 0}},
        {"gcc Pair copy", compiled_copy_at, {4, 0, 0}},
        {"gcc Pair double sum", compiled_double_at, {4, 0, 0}},
    };
    for (const Load& l : loads) {
        for (unsigned off : l.offs) {
            if (off == 0) continue;
            char name[64];
            std::snprintf(name, sizeof name, "%s at +%u", l.name, off);
            run_case(name, [&] { return l.fn(off); });
        }
    }

    line("done");
    sceKernelExitProcess(0);
    return 0;
}
