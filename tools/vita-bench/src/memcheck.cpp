// Reports the app's free memory per type to ux0:data/nfbench/memory-<titleid>.txt
// and exits. Built twice (CMakeLists.txt): with the default memory budget and with
// the extended memory mode (ATTRIBUTE2=12), under different title IDs.
//
// It also checks whether the physically contiguous pool is part of the user
// budget or separate: it allocates from one type and reads the others' free
// sizes, then fills user memory completely and tries a contiguous block.

#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>

#include <cstdio>
#include <vector>

extern "C" {
// Keep newlib's heap out of the way so the free sizes are close to the full budget.
unsigned int _newlib_heap_size_user = 1 * 1024 * 1024;
}

namespace {

constexpr SceSize kMiB = 1024 * 1024;
FILE* g_out = nullptr;

void report(const char* when) {
    SceKernelFreeMemorySizeInfo mem{};
    mem.size = sizeof mem;
    int err = sceKernelGetFreeMemorySize(&mem);
    std::fprintf(g_out, "%-44s user %6.1f  cdram %6.1f  phycont %6.1f MiB%s\n", when, mem.size_user / double(kMiB),
                 mem.size_cdram / double(kMiB), mem.size_phycont / double(kMiB), err ? "  (query failed)" : "");
}

SceUID alloc(const char* name, SceKernelMemBlockType type, SceSize size) {
    SceUID uid = sceKernelAllocMemBlock(name, type, size, nullptr);
    std::fprintf(g_out, "  alloc %s %u MiB: %s (0x%08X)\n", name, unsigned(size / kMiB), uid >= 0 ? "ok" : "failed",
                 unsigned(uid));
    return uid;
}

}  // namespace

int main() {
    char path[64];
    std::snprintf(path, sizeof path, "ux0:data/nfbench/memory-%s.txt", NFMEM_TITLEID);
    sceIoMkdir("ux0:data/nfbench", 0777);
    g_out = std::fopen(path, "w");
    if (!g_out) sceKernelExitProcess(1);

    std::fprintf(g_out, "title %s, extended memory requested: %s, newlib heap 1 MiB\n\n", NFMEM_TITLEID,
                 NFMEM_EXTENDED ? "yes" : "no");
    report("start");

    // 1. Allocate from one pool, see which pools shrink.
    SceUID phy = alloc("phycont", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW, 16 * kMiB);
    report("with 16 MiB phycont allocated");
    if (phy >= 0) sceKernelFreeMemBlock(phy);
    SceUID user = alloc("user", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, 64 * kMiB);
    report("with 64 MiB user allocated");
    if (user >= 0) sceKernelFreeMemBlock(user);

    // 2. Fill user memory, then try a contiguous block.
    std::vector<SceUID> blocks;
    blocks.reserve(512);
    SceSize filled = 0;
    for (SceSize step : {16 * kMiB, kMiB}) {
        for (;;) {
            SceUID uid = sceKernelAllocMemBlock("fill", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, step, nullptr);
            if (uid < 0) break;
            blocks.push_back(uid);
            filled += step;
        }
    }
    std::fprintf(g_out, "\nfilled user memory: %u MiB in %u blocks\n", unsigned(filled / kMiB), unsigned(blocks.size()));
    report("with user memory full");
    phy = alloc("phycont", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW, 16 * kMiB);
    report("with user full + 16 MiB phycont");
    if (phy >= 0) sceKernelFreeMemBlock(phy);
    for (SceUID uid : blocks) sceKernelFreeMemBlock(uid);
    report("after freeing everything");

    std::fclose(g_out);
    sceKernelExitProcess(0);
    return 0;
}
