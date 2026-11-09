#ifndef _DOLPHIN_OSEXEC_H_
#define _DOLPHIN_OSEXEC_H_

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    BOOL valid;
    u32 restartCode;
    u32 bootDol;
    void* regionStart;
    void* regionEnd;
    int argsUseDefault;
    void* argsAddr;
} OSExecParams;

typedef int (*appGetNextCallback)(void*, u32*, u32*);
typedef void (*appInitCallback)(void (*)(char*));
typedef void* (*appGetEntryCallback)();
typedef void (*AppLoaderCallback)(appInitCallback*, appGetNextCallback*, appGetEntryCallback*);

#ifdef __MWERKS__
OSExecParams* __OSExecParams AT_ADDRESS(0x800030F0);
s32 __OSAppLoaderOffset AT_ADDRESS(0x800030F4);
#else
// With MWCC these live at fixed addresses in low memory. Elsewhere AT_ADDRESS expands to nothing,
// which would make this header *define* the variables in every translation unit that includes it
// (duplicate symbols at link time), so they are plain declarations here and defined exactly once
// by the platform layer (src/nightfall/platform/os_globals.cpp).
extern OSExecParams* __OSExecParams;
extern s32 __OSAppLoaderOffset;
#endif

void OSExecv(const char* dolfile, const char** argv);
void OSExecl(const char* dolfile, const char* arg0, ...);

#ifdef __cplusplus
}
#endif

#endif
