// Tests for JKernel/JKRDecomp.cpp: the decompression thread's handling of a queued command.
//
// What is under test
// ------------------
// JKRDecomp::run() is the body of the decompression thread. It takes a JKRDecompCommand from the
// message queue, decompresses `mSrcBuffer` into `mDstBuffer`, and then reports completion in one
// of three ways, depending on the command:
//   * `field_0x20 != 0`   -> hand over to the ARAM piece or do nothing more;
//   * `mCallback` set     -> call mCallback(command) ("async" mode);
//   * otherwise           -> post a message on the command's queue (`field_0x1c`, or its own).
//
// Why these tests exist
// ---------------------
// The callback receives the command as a single integer argument. That argument used to be a u32,
// so on a 64-bit host the command's address was truncated and the callback worked on the wrong
// object. It is now uintptr_t. The tests check that the callback receives exactly the address of
// the command it was queued with, and cover the other two completion paths and the decompression
// itself, since they share the loop.
//
// How it runs: run() is an endless loop that blocks on OSReceiveMessage, and the SDK's message
// queue is not built on the host. The tests supply a scripted OSReceiveMessage that hands out the
// queued commands one by one and then jumps out of the loop with longjmp. OSSendMessage records
// what the code posts. The thread object itself is never constructed: run() only touches static
// state, so it is called (non-virtually) on zeroed storage. That is technically undefined
// behaviour, confined to this file; run() is private, hence the access hack around the header.
//
// Not covered: field_0x20 == 1 (hands the command to the ARAM piece, which needs the ARAM
// subsystem).

#include <gtest/gtest.h>

// Standard headers first, so the access hack below only applies to project headers.
#include <csetjmp>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#define private public
#include "JSystem/JKernel/JKRDecomp.h"
#undef private

#include "support/load_time_stubs.hpp"
#include "support/os_stubs.hpp"

// --- Scripted message queue ------------------------------------------------------------------

namespace {

std::jmp_buf gLeaveLoop;
std::vector<void*> gIncoming;                                   // what OSReceiveMessage returns
size_t gNextIncoming = 0;
std::vector<std::pair<OSMessageQueue*, void*>> gPosted;         // what OSSendMessage was given

void ResetScript(std::vector<void*> incoming) {
    gIncoming = std::move(incoming);
    gNextIncoming = 0;
    gPosted.clear();
}

// Runs the decompression loop until the scripted messages are used up.
void RunDecompressionLoop() {
    alignas(JKRDecomp) unsigned char storage[sizeof(JKRDecomp)] = {};
    auto* thread = reinterpret_cast<JKRDecomp*>(storage);
    if (setjmp(gLeaveLoop) == 0) {
        thread->JKRDecomp::run();  // non-virtual call: the object has no vtable
    }
}

// What the callback was called with.
std::vector<uintptr_t> gCallbackArgs;
void RecordingCallback(uintptr_t argument) { gCallbackArgs.push_back(argument); }

}  // namespace

extern "C" {
void OSInitMessageQueue(OSMessageQueue*, void*, s32) {}

int OSReceiveMessage(OSMessageQueue*, void* message, s32) {
    if (gNextIncoming == gIncoming.size()) {
        longjmp(gLeaveLoop, 1);
    }
    *static_cast<void**>(message) = gIncoming[gNextIncoming++];
    return 1;
}

int OSSendMessage(OSMessageQueue* queue, void* message, s32) {
    gPosted.emplace_back(queue, message);
    return 1;
}
}

namespace {

// A command for data that is not compressed, so the loop's decode step has nothing to do.
JKRDecompCommand MakeCommand(u8* src, u8* dst, u32 srcLength, u32 dstLength) {
    JKRDecompCommand command;
    command.mSrcBuffer = src;
    command.mDstBuffer = dst;
    command.mSrcLength = srcLength;
    command.mDstLength = dstLength;
    command.mCallback = nullptr;
    command.field_0x1c = nullptr;
    command.field_0x20 = 0;
    return command;
}

class JKRDecompTest : public ::testing::Test {
protected:
    void SetUp() override { gCallbackArgs.clear(); }
    u8 src[32] = {};
    u8 dst[32] = {};
};

}  // namespace

// --- The callback path -----------------------------------------------------------------------

// The point of the regression: the argument is the command's whole address.
TEST_F(JKRDecompTest, CallbackReceivesTheAddressOfTheCommand) {
    JKRDecompCommand command = MakeCommand(src, dst, sizeof(src), sizeof(dst));
    command.mCallback = RecordingCallback;
    ResetScript({&command});

    RunDecompressionLoop();

    ASSERT_EQ(gCallbackArgs.size(), 1u);
    EXPECT_EQ(gCallbackArgs[0], reinterpret_cast<uintptr_t>(&command));
    EXPECT_EQ(reinterpret_cast<JKRDecompCommand*>(gCallbackArgs[0]), &command);
}

// The callback is the only notification: nothing is posted to a queue.
TEST_F(JKRDecompTest, CallbackPathDoesNotPostToAQueue) {
    JKRDecompCommand command = MakeCommand(src, dst, sizeof(src), sizeof(dst));
    command.mCallback = RecordingCallback;
    ResetScript({&command});

    RunDecompressionLoop();

    EXPECT_TRUE(gPosted.empty());
}

TEST_F(JKRDecompTest, EachQueuedCommandGetsItsOwnCallback) {
    u8 srcB[32] = {}, dstB[32] = {};
    JKRDecompCommand first = MakeCommand(src, dst, sizeof(src), sizeof(dst));
    JKRDecompCommand second = MakeCommand(srcB, dstB, sizeof(srcB), sizeof(dstB));
    first.mCallback = RecordingCallback;
    second.mCallback = RecordingCallback;
    ResetScript({&first, &second});

    RunDecompressionLoop();

    ASSERT_EQ(gCallbackArgs.size(), 2u);
    EXPECT_EQ(gCallbackArgs[0], reinterpret_cast<uintptr_t>(&first));
    EXPECT_EQ(gCallbackArgs[1], reinterpret_cast<uintptr_t>(&second));
}

// --- The other completion paths --------------------------------------------------------------

// No callback: the command's own queue is told the work is done.
TEST_F(JKRDecompTest, WithoutACallbackItPostsOnTheCommandsOwnQueue) {
    JKRDecompCommand command = MakeCommand(src, dst, sizeof(src), sizeof(dst));
    ResetScript({&command});

    RunDecompressionLoop();

    ASSERT_EQ(gPosted.size(), 1u);
    EXPECT_EQ(gPosted[0].first, &command.mMessageQueue);
    EXPECT_EQ(gPosted[0].second, reinterpret_cast<void*>(1));
}

// ...or on a queue the caller supplied.
TEST_F(JKRDecompTest, WithoutACallbackItPostsOnTheCallersQueueIfThereIsOne) {
    OSMessageQueue callersQueue;
    JKRDecompCommand command = MakeCommand(src, dst, sizeof(src), sizeof(dst));
    command.field_0x1c = &callersQueue;
    ResetScript({&command});

    RunDecompressionLoop();

    ASSERT_EQ(gPosted.size(), 1u);
    EXPECT_EQ(gPosted[0].first, &callersQueue);
}

// A non-zero field_0x20 other than 1 means "nothing to report".
TEST_F(JKRDecompTest, ACommandMarkedForSomeoneElseReportsNothing) {
    JKRDecompCommand command = MakeCommand(src, dst, sizeof(src), sizeof(dst));
    command.mCallback = RecordingCallback;
    command.field_0x20 = 2;
    ResetScript({&command});

    RunDecompressionLoop();

    EXPECT_TRUE(gCallbackArgs.empty());
    EXPECT_TRUE(gPosted.empty());
}

// --- Decompression ---------------------------------------------------------------------------

// A minimal Yaz0 stream: 16-byte header ("Yaz0", big-endian decompressed size, 8 reserved bytes),
// then one group whose flag byte 0xFF marks eight literal bytes.
//
// Note the odd names in the command: for Yaz0, `mSrcLength` is the number of output bytes to
// produce and `mDstLength` is the number of leading output bytes to skip (so a caller can decode
// the middle of a file), not buffer sizes.
TEST_F(JKRDecompTest, DecompressesBeforeReporting) {
    const u8 compressed[] = {'Y', 'a', 'z', '0', 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0,
                             0xFF, 'J', 'S', 'y', 's', 't', 'e', 'm', '!'};
    std::memcpy(src, compressed, sizeof(compressed));
    JKRDecompCommand command = MakeCommand(src, dst, /*bytes to produce=*/8, /*bytes to skip=*/0);
    command.mCallback = RecordingCallback;
    ResetScript({&command});

    RunDecompressionLoop();

    EXPECT_EQ(std::memcmp(dst, "JSystem!", 8), 0);
    EXPECT_EQ(gCallbackArgs.size(), 1u);  // and only then is the callback told
}

// --- The type of the argument ----------------------------------------------------------------

static_assert(sizeof(uintptr_t) >= sizeof(void*), "uintptr_t can hold a pointer");
static_assert(std::is_same_v<JKRDecompCommand::AsyncCallback, void (*)(uintptr_t)>,
              "the callback must receive a pointer-sized argument, not a u32");
