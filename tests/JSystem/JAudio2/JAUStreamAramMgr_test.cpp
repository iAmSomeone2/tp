// Tests for JAudio2/JAUStreamAramMgr.h: JAUStreamStaticAramMgr_, which hands the streaming code
// fixed chunks of "ARAM" (the GameCube's audio memory) and takes them back by address.
//
// What is under test
// ------------------
// reserveAram() carves a number of equal chunks out of a JASHeap. newStreamAram() then returns
// the base of the first unused chunk, and deleteStreamAram() marks the chunk whose base equals
// the given address as unused again. Callers (JAIStreamMgr) keep the address from the allocation
// and pass it back.
//
// Why these tests exist
// ---------------------
// deleteStreamAram() used to take a u32 and compare it with `(uintptr_t)base`. On a 64-bit host
// the u32 cannot hold an address above 4 GiB, so a chunk was never found and never given back:
// every stream leaked its ARAM chunk. The parameter is now uintptr_t, and the interface it
// overrides (JAIStreamAramMgr::deleteStreamAram) matches. A mismatch between the two signatures
// compiles but silently leaves the class abstract or the override unused, which these tests and
// the instantiation below would catch.
//
// Addresses: nothing here is ever dereferenced, so the tests hand the manager made-up root
// addresses, one below 4 GiB (what the GameCube would have) and one above (what a 64-bit host
// produces for ordinary memory).
//
// How it is built: the manager is a header-only template, and JASHeap (what it allocates from) is
// the real JAudio2 code, linked (see tests/JSystem/CMakeLists.txt).

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

#include "JSystem/JAudio2/JAUStreamAramMgr.h"

#include "support/load_time_stubs.hpp"
#include "support/os_stubs.hpp"

// reserveAram() sizes each chunk as (count argument) * JASAramStream::getBlockSize(). The real
// value is set when the streaming system starts up, which the tests do not do, so they define the
// variable themselves. (If JASAramStream.cpp is ever linked into these tests, remove this.)
u32 JASAramStream::sBlockSize = 0x2000;

namespace {

constexpr uintptr_t kLowRoot = 0x80100000u;          // a GameCube-style address (fits 32 bits)
constexpr uintptr_t kHighRoot = uintptr_t{0x123400000};  // does not fit 32 bits (64-bit hosts only)
constexpr u32 kChunkBytes = 0x2000;                  // sBlockSize * 1, already 32-byte aligned
constexpr int kChunks = 3;

using Manager = JAUStreamStaticAramMgr_<4>;

// A root heap at a made-up address, with a manager that has reserved kChunks chunks from it.
struct Fixture {
    explicit Fixture(uintptr_t rootAddress) {
        root.initRootHeap(reinterpret_cast<void*>(rootAddress), 0x10000);
        manager.reserveAram(&root, kChunks, 1);
    }
    JASHeap root;
    Manager manager;
};

}  // namespace

// The manager is only usable if it overrides every pure virtual of its interface with the exact
// signature. This fails to compile (not just at run time) if a signature drifts again.
static_assert(!std::is_abstract_v<Manager>, "deleteStreamAram/newStreamAram must override the interface");

class JAUStreamAramMgrTest : public ::testing::TestWithParam<uintptr_t> {};

// Both address ranges behave identically. The 64-bit one is skipped on 32-bit hosts, where the
// address does not exist.
INSTANTIATE_TEST_SUITE_P(Addresses, JAUStreamAramMgrTest, ::testing::Values(kLowRoot, kHighRoot));

#define SKIP_IF_ADDRESS_DOES_NOT_FIT(root)                                       \
    do {                                                                         \
        if (reinterpret_cast<uintptr_t>(reinterpret_cast<void*>(root)) != (root)) { \
            GTEST_SKIP() << "address does not fit a pointer on this host";       \
        }                                                                        \
    } while (0)

TEST_P(JAUStreamAramMgrTest, ReserveCreatesTheRequestedChunks) {
    SKIP_IF_ADDRESS_DOES_NOT_FIT(GetParam());
    Fixture f(GetParam());
    EXPECT_TRUE(f.manager.isAramReserved());
    EXPECT_FALSE(f.manager.isStreamUsingAram());
}

TEST_P(JAUStreamAramMgrTest, NewStreamAramHandsOutChunksInOrder) {
    SKIP_IF_ADDRESS_DOES_NOT_FIT(GetParam());
    Fixture f(GetParam());
    for (int i = 0; i < kChunks; i++) {
        u32 size = 0;
        void* chunk = f.manager.newStreamAram(&size);
        ASSERT_NE(chunk, nullptr) << "chunk " << i;
        EXPECT_EQ(reinterpret_cast<uintptr_t>(chunk), GetParam() + i * kChunkBytes);
        EXPECT_EQ(size, kChunkBytes);
    }
    EXPECT_TRUE(f.manager.isStreamUsingAram());
}

TEST_P(JAUStreamAramMgrTest, NewStreamAramReturnsNullWhenEveryChunkIsInUse) {
    SKIP_IF_ADDRESS_DOES_NOT_FIT(GetParam());
    Fixture f(GetParam());
    u32 size = 0;
    for (int i = 0; i < kChunks; i++) {
        ASSERT_NE(f.manager.newStreamAram(&size), nullptr);
    }
    EXPECT_EQ(f.manager.newStreamAram(&size), nullptr);
}

// The point of the regression: the address handed out can be handed back, whatever its width.
TEST_P(JAUStreamAramMgrTest, DeleteStreamAramReturnsAChunkGivenItsAddress) {
    SKIP_IF_ADDRESS_DOES_NOT_FIT(GetParam());
    Fixture f(GetParam());
    u32 size = 0;
    void* chunk = f.manager.newStreamAram(&size);
    ASSERT_NE(chunk, nullptr);
    ASSERT_TRUE(f.manager.isStreamUsingAram());

    EXPECT_TRUE(f.manager.deleteStreamAram(reinterpret_cast<uintptr_t>(chunk)));
    EXPECT_FALSE(f.manager.isStreamUsingAram());
}

TEST_P(JAUStreamAramMgrTest, AFreedChunkCanBeAllocatedAgain) {
    SKIP_IF_ADDRESS_DOES_NOT_FIT(GetParam());
    Fixture f(GetParam());
    u32 size = 0;
    void* first = f.manager.newStreamAram(&size);
    void* second = f.manager.newStreamAram(&size);
    ASSERT_TRUE(f.manager.deleteStreamAram(reinterpret_cast<uintptr_t>(first)));

    EXPECT_EQ(f.manager.newStreamAram(&size), first);  // the lowest free chunk is reused
    EXPECT_NE(second, first);
}

TEST_P(JAUStreamAramMgrTest, DeleteStreamAramRejectsAddressesThatAreNotChunks) {
    SKIP_IF_ADDRESS_DOES_NOT_FIT(GetParam());
    Fixture f(GetParam());
    u32 size = 0;
    void* chunk = f.manager.newStreamAram(&size);
    const uintptr_t address = reinterpret_cast<uintptr_t>(chunk);

    EXPECT_FALSE(f.manager.deleteStreamAram(address + 0x10));  // inside a chunk, not its base
    EXPECT_FALSE(f.manager.deleteStreamAram(address - 0x20));  // before the first chunk
    EXPECT_FALSE(f.manager.deleteStreamAram(0));
    EXPECT_TRUE(f.manager.isStreamUsingAram());                // the real chunk is still held
}

TEST_P(JAUStreamAramMgrTest, DeletingTheSameChunkTwiceFailsTheSecondTime) {
    SKIP_IF_ADDRESS_DOES_NOT_FIT(GetParam());
    Fixture f(GetParam());
    u32 size = 0;
    const uintptr_t address = reinterpret_cast<uintptr_t>(f.manager.newStreamAram(&size));
    EXPECT_TRUE(f.manager.deleteStreamAram(address));
    EXPECT_FALSE(f.manager.deleteStreamAram(address));
}

// An unreserved manager has nothing to hand out.
TEST(JAUStreamAramMgr, WithoutReservationThereAreNoChunks) {
    Manager manager;
    u32 size = 0;
    EXPECT_FALSE(manager.isAramReserved());
    EXPECT_EQ(manager.newStreamAram(&size), nullptr);
    EXPECT_FALSE(manager.deleteStreamAram(kLowRoot));
}
