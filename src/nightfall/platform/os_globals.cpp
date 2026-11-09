// Host definitions of the Dolphin SDK globals that live at fixed addresses on a GameCube.
//
// On the console these are variables at known addresses in low memory, which Metrowerks places
// with `type name : (address);`. Everywhere else the SDK headers only *declare* them (see
// libs/dolphin/include/dolphin/os/OSExec.h), and this is the one translation unit that defines
// them, so the engine libraries can be linked together without duplicate symbols.
//
// This file is the start of the platform layer that will replace the SDK implementation (see
// libs/dolphin/CMakeLists.txt). Further globals the host needs to own belong here too.

#include <dolphin/os.h>
#include <dolphin/os/OSExec.h>

OSExecParams* __OSExecParams = nullptr;
s32 __OSAppLoaderOffset = 0;

// The GameCube's clocks, which its boot code stores in low memory: a 486 MHz CPU and a 162 MHz bus
// (the timer, OS_TIMER_CLOCK, runs at a quarter of the bus clock). Game code derives time constants
// from these in global initialisers, so they must have these values before main().
u32 __OSBusClock = 162000000;
u32 __OSCoreClock = 486000000;
