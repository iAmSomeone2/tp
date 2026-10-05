// Minimal stand-ins for the few Dolphin OS calls that the JSystem code under test reaches.
//
// The Dolphin SDK implementation is deliberately not part of the host build (see
// libs/dolphin/CMakeLists.txt), so anything a test executes must either avoid the SDK or be
// stubbed here. Mutexes are no-ops because the tests are single-threaded; OSReport discards its
// output.
//
// Include this header exactly ONCE per test executable, after the project headers: it defines
// functions with external linkage.
#pragma once

#include <os.h>

extern "C" {
void OSInitMutex(OSMutex*) {}
void OSLockMutex(OSMutex*) {}
void OSUnlockMutex(OSMutex*) {}
void OSReport(const char*, ...) {}
}
