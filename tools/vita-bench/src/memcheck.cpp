// Reports the app's free memory per type to ux0:data/nfbench/memory-<titleid>.txt
// and exits. Built twice (CMakeLists.txt): with the default memory budget and with
// the extended memory mode (ATTRIBUTE2=12), under different title IDs.

#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>

#include <cstdio>

extern "C" {
// Keep newlib's heap out of the way so the free sizes are close to the full budget.
unsigned int _newlib_heap_size_user = 1 * 1024 * 1024;
}

int main() {
    SceKernelFreeMemorySizeInfo mem{};
    mem.size = sizeof mem;
    int err = sceKernelGetFreeMemorySize(&mem);

    char path[64];
    std::snprintf(path, sizeof path, "ux0:data/nfbench/memory-%s.txt", NFMEM_TITLEID);
    sceIoMkdir("ux0:data/nfbench", 0777);
    if (FILE* f = std::fopen(path, "w")) {
        std::fprintf(f, "title %s, extended memory requested: %s, newlib heap 1 MiB\n", NFMEM_TITLEID,
                     NFMEM_EXTENDED ? "yes" : "no");
        std::fprintf(f, "sceKernelGetFreeMemorySize: 0x%08X\n", err);
        std::fprintf(f, "user    %8.1f MiB\ncdram   %8.1f MiB\nphycont %8.1f MiB\n", mem.size_user / 1048576.0,
                     mem.size_cdram / 1048576.0, mem.size_phycont / 1048576.0);
        std::fclose(f);
    }
    sceKernelExitProcess(0);
    return 0;
}
