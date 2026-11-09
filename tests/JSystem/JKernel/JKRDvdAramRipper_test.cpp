// Tests for JKernel/JKRDvdAramRipper.cpp: the callback argument of asynchronous loads.
//
// What is under test
// ------------------
// JKRDvdAramRipper::loadToAram_Async() takes an optional callback that it calls, with the command
// describing the load, once the data has been queued. The command is passed as a single integer.
//
// Why these tests exist
// ---------------------
// That integer used to be a u32, which cannot hold a command address on a 64-bit host. The
// argument, the callback type and the command's `mAddress` field are now uintptr_t.
//
// What these tests cannot do
// --------------------------
// The call itself sits inside loadToAram_Async(), which needs the DVD file system, a JKRDvdFile and
// the ARAM streaming code. None of that exists on the host (the SDK implementation is not built),
// so the call path is *not* exercised here. What can be checked without it is the contract the
// call relies on: the callback type carries a whole address, and an address survives the round
// trip through that argument. If the DVD/ARAM layer is ever given a host implementation, add a test
// that runs loadToAram_Async() with a callback and checks it receives the command's address, in the
// style of JKRDecomp_test.cpp.

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

#include "JSystem/JKernel/JKRDvdAramRipper.h"

#include "support/load_time_stubs.hpp"
#include "support/os_stubs.hpp"

namespace {

std::vector<uintptr_t> gReceived;
void Receive(uintptr_t argument) { gReceived.push_back(argument); }

}  // namespace

// The callback's parameter is pointer-sized.
static_assert(std::is_same_v<decltype(JKRADCommand::mCallback), void (*)(uintptr_t)>,
              "an async-load callback must receive a pointer-sized argument, not a u32");
static_assert(sizeof(decltype(JKRADCommand::mAddress)) >= sizeof(void*),
              "mAddress holds an address and must be pointer-sized");

// A command's address survives being passed through the callback argument and back.
TEST(JKRDvdAramRipper, CallbackArgumentRoundTripsACommandAddress) {
    gReceived.clear();
    JKRADCommand command;
    command.mCallback = Receive;

    command.mCallback(reinterpret_cast<uintptr_t>(&command));

    ASSERT_EQ(gReceived.size(), 1u);
    EXPECT_EQ(reinterpret_cast<JKRADCommand*>(gReceived[0]), &command);
}

// An address that does not fit in 32 bits is not truncated by the argument. Skipped where no such
// address exists.
TEST(JKRDvdAramRipper, CallbackArgumentKeepsAddressesAbove4GiB) {
    if (sizeof(void*) < 8) {
        GTEST_SKIP() << "addresses above 4 GiB do not exist on this host";
    }
    gReceived.clear();
    JKRADCommand command;
    command.mCallback = Receive;

    const uintptr_t large = uintptr_t{0x123456789};
    command.mCallback(large);

    ASSERT_EQ(gReceived.size(), 1u);
    EXPECT_EQ(gReceived[0], large);
}
