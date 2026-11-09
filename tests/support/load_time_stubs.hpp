// Stand-ins for SDK symbols that JKernel/JUtility objects reference in a way the loader must
// resolve at start-up, even though the tests never use them.
//
// The lenient link described in tests/CMakeLists.txt (LENIENT_LINK) lets a test leave SDK
// *function calls* unresolved, because a call is only bound if it is made. Two kinds of reference
// are bound immediately and so need a definition:
//   * data (here the GX render-mode table and an FPU-exception mask), and
//   * a function whose address is taken (here ARAlloc and DCFlushRange, held in tables).
// Everything below does nothing or is zeroed, and the tests never read it. The GX table is not
// defined anywhere in this repository yet (the GX implementation is not built on the host).
//
// Include at most once per test executable, after the SDK headers have been seen.
#pragma once

#include <ar.h>
#include <gx.h>
#include <os.h>

GXRenderModeObj GXNtsc480Int = {};
u32 __OSFpscrEnableBits = 0;

u32 ARAlloc(u32) { return 0; }
void DCFlushRange(void*, u32) {}
