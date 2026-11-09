// Tests for JAudio2/JASHeapCtrl.cpp: JASHeap, the sub-allocator JAudio2 uses for sound memory.
//
// What is under test
// ------------------
// A JASHeap is a node in a tree. A "root" heap owns a block of memory; child heaps are carved out
// of their mother's block with JASHeap::alloc() (from the front) or allocTail() (from the back)
// and returned with JASHeap::free(). Sizes are rounded up to 32 bytes. When the front of the
// mother is exhausted, alloc() falls back to searching the gaps left by freed children and picks
// the smallest gap that fits (best fit).
//
// Why these tests exist
// ---------------------
// For the 64-bit port, JASHeap::alloc() and initRootHeap() were rewritten to do their address
// arithmetic in uintptr_t instead of u32, and OSRoundUp32B() (os.h) was changed to match. The
// gap search in particular used to compare 32-bit differences against an s32 "infinity" of -1.
// The tests below pin the behaviour from three directions:
//   * hand-written scenarios for each documented behaviour;
//   * the best-fit gap search, which only runs once the mother is full;
//   * a randomised comparison with a verbatim copy of the ORIGINAL alloc() (the version before
//     the port), run on identical operation sequences. Any divergence in result, position or
//     size fails the test.
//
// Memory is never dereferenced: JASHeap only does address arithmetic, so the tests give heaps an
// ordinary buffer where convenient and a made-up address where that is clearer.
//
// How it is built: the test links the real JAudio2 code (see tests/JSystem/CMakeLists.txt).

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "JSystem/JAudio2/JASHeapCtrl.h"

#include "support/load_time_stubs.hpp"
#include "support/os_stubs.hpp"

namespace {

// An aligned buffer that a root heap can own.
struct Arena {
    explicit Arena(size_t size) : size(size), base(static_cast<u8*>(std::aligned_alloc(64, size))) {}
    ~Arena() { std::free(base); }
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    size_t size;
    u8* base;
};

// Offset of a child's block from the start of its mother's memory.
std::ptrdiff_t OffsetIn(const JASHeap& mother, const JASHeap& child) {
    return child.mBase - mother.mBase;
}

}  // namespace

// --- initRootHeap ----------------------------------------------------------------------------

TEST(JASHeap, NewHeapOwnsNothing) {
    JASHeap heap;
    EXPECT_FALSE(heap.isAllocated());
    EXPECT_EQ(heap.getSize(), 0u);
}

// The root's base is rounded up to a 32-byte boundary and the size shrinks by the bytes skipped.
TEST(JASHeap, InitRootHeapAlignsBaseTo32Bytes) {
    Arena arena(4096);
    JASHeap root;
    u8* misaligned = arena.base + 5;
    root.initRootHeap(misaligned, 4000);

    const auto rounded = (reinterpret_cast<uintptr_t>(misaligned) + 31) & ~uintptr_t{31};
    EXPECT_EQ(reinterpret_cast<uintptr_t>(root.getBase()), rounded);
    EXPECT_EQ(root.getBase(), arena.base + 32);
    EXPECT_EQ(root.getSize(), 4000u - (rounded - reinterpret_cast<uintptr_t>(misaligned)));
}

TEST(JASHeap, InitRootHeapKeepsAnAlignedBase) {
    Arena arena(4096);
    JASHeap root;
    root.initRootHeap(arena.base, 4096);
    EXPECT_EQ(root.getBase(), arena.base);
    EXPECT_EQ(root.getSize(), 4096u);
}

// --- alloc: the front of the mother ----------------------------------------------------------

// Sizes are rounded up to 32 bytes and consecutive blocks follow each other.
TEST(JASHeap, AllocTakesRoundedBlocksFromTheFront) {
    Arena arena(4096);
    JASHeap root, a, b;
    root.initRootHeap(arena.base, 4096);

    ASSERT_TRUE(a.alloc(&root, 100));
    ASSERT_TRUE(b.alloc(&root, 200));
    EXPECT_EQ(OffsetIn(root, a), 0);
    EXPECT_EQ(a.getSize(), 128u);
    EXPECT_EQ(OffsetIn(root, b), 128);
    EXPECT_EQ(b.getSize(), 224u);
}

TEST(JASHeap, AllocFailsWhenTheMotherIsTooSmall) {
    Arena arena(1024);
    JASHeap root, big;
    root.initRootHeap(arena.base, 1024);
    EXPECT_FALSE(big.alloc(&root, 1025));
    EXPECT_FALSE(big.isAllocated());
}

TEST(JASHeap, AllocCanUseTheWholeMother) {
    Arena arena(1024);
    JASHeap root, all;
    root.initRootHeap(arena.base, 1024);
    ASSERT_TRUE(all.alloc(&root, 1024));
    EXPECT_EQ(all.getSize(), 1024u);
}

TEST(JASHeap, AllocRefusesAHeapThatIsAlreadyAllocated) {
    Arena arena(1024);
    JASHeap root, a;
    root.initRootHeap(arena.base, 1024);
    ASSERT_TRUE(a.alloc(&root, 64));
    EXPECT_FALSE(a.alloc(&root, 64));
}

TEST(JASHeap, AllocFromAMotherThatOwnsNothingFails) {
    JASHeap empty_mother, child;
    EXPECT_FALSE(child.alloc(&empty_mother, 64));
}

// --- alloc: reusing freed gaps (best fit) ----------------------------------------------------

// Layout of a completely full 1024-byte mother:
//     [ keep1 128 ][ big 512 ][ keep2 128 ][ small 128 ][ keep3 128 ]
//       0            128        640          768          896
// Freeing `big` and `small` leaves two gaps (512 and 128 bytes). With the front exhausted, a new
// allocation must choose the smallest gap that fits it.
class JASHeapGapTest : public ::testing::Test {
protected:
    void SetUp() override {
        arena = std::make_unique<Arena>(1024);
        root.initRootHeap(arena->base, 1024);
        ASSERT_TRUE(keep1.alloc(&root, 128));
        ASSERT_TRUE(big.alloc(&root, 512));
        ASSERT_TRUE(keep2.alloc(&root, 128));
        ASSERT_TRUE(small.alloc(&root, 128));
        ASSERT_TRUE(keep3.alloc(&root, 128));
        ASSERT_EQ(OffsetIn(root, big), 128);
        ASSERT_EQ(OffsetIn(root, small), 768);
        ASSERT_TRUE(big.free());
        ASSERT_TRUE(small.free());
    }

    std::unique_ptr<Arena> arena;
    JASHeap root, keep1, big, keep2, small, keep3;
};

TEST_F(JASHeapGapTest, ASmallRequestTakesTheSmallestGapThatFits) {
    JASHeap request;
    ASSERT_TRUE(request.alloc(&root, 100));  // rounds to 128: fits both gaps, 128 is the tighter
    EXPECT_EQ(OffsetIn(root, request), 768);
}

TEST_F(JASHeapGapTest, ALargeRequestTakesTheLargeGap) {
    JASHeap request;
    ASSERT_TRUE(request.alloc(&root, 400));  // rounds to 416: only the 512-byte gap fits
    EXPECT_EQ(OffsetIn(root, request), 128);
}

TEST_F(JASHeapGapTest, ARequestLargerThanEveryGapFails) {
    JASHeap request;
    EXPECT_FALSE(request.alloc(&root, 600));
}

TEST_F(JASHeapGapTest, ASecondRequestUsesTheRemainingGap) {
    JASHeap first, second;
    ASSERT_TRUE(first.alloc(&root, 128));
    ASSERT_TRUE(second.alloc(&root, 128));
    EXPECT_EQ(OffsetIn(root, first), 768);   // tightest gap first
    EXPECT_EQ(OffsetIn(root, second), 128);  // then the start of the big gap
}

// --- Comparison with the original algorithm --------------------------------------------------

namespace {

// Verbatim copy of JASHeap::alloc() as it was before the 64-bit port (git history of
// libs/JSystem/src/JAudio2/JASHeapCtrl.cpp), with two mechanical changes: 32-bit types are
// spelled out (the originals were u32 and s32 on a 32-bit target) and the mutex lock is omitted.
// Pointers are narrowed through uintptr_t first, which on a 32-bit target is a no-op.
using u32_orig = std::uint32_t;
using s32_orig = std::int32_t;
#define ORIGINAL_ROUNDUP32B(x) (((u32_orig)(x) + 32 - 1) & ~(32 - 1))

bool OriginalAlloc(JASHeap* self, JASHeap* mother, u32_orig param_1) {
    if (self->isAllocated()) {
        return 0;
    }
    if (!mother->isAllocated()) {
        return 0;
    }
    param_1 = ORIGINAL_ROUNDUP32B(param_1);
    u32_orig local_28 = mother->getCurOffset();
    u32_orig local_2c = mother->getTailOffset();
    if (local_28 + param_1 <= local_2c) {
        mother->insertChild(self, mother->getTailHeap(), mother->mBase + local_28, param_1, false);
        return 1;
    }
    s32_orig r27 = -1;
    u8* r29 = mother->mBase;
    bool local_43 = false;
    JASHeap* local_30 = NULL;
    void* local_34;
    JSUTreeIterator<JASHeap> it;
    for (it = mother->mTree.getFirstChild(); it != mother->mTree.getEndChild(); it++) {
        if (r29 >= mother->mBase + local_2c) {
            break;
        }
        u32_orig local_3c = u32_orig(uintptr_t(it->mBase)) - u32_orig(uintptr_t(r29));
        if (local_3c >= param_1 && local_3c < (u32_orig)r27) {  // u32 vs s32: compared unsigned
            local_30 = *it;
            local_34 = r29;
            r27 = local_3c;
            local_43 = true;
        }
        u32_orig r25 = it->mSize;
        r29 = (u8*)it->mBase + r25;
    }
    if (r29 != mother->mBase && r29 < mother->mBase + local_2c) {
        u32_orig local_40 = mother->mBase + mother->mSize - r29;
        if (local_40 >= param_1 && local_40 < (u32_orig)r27) {
            local_30 = NULL;
            local_34 = r29;
            r27 = local_40;
            local_43 = true;
        }
    }
    if (!local_43) {
        return 0;
    }
    mother->insertChild(self, local_30, local_34, param_1, false);
    return 1;
}

// Two independent heaps trees that are driven identically; `kids` are the children of `root`.
struct Universe {
    static constexpr int kChildren = 24;
    explicit Universe(size_t size) : arena(size), kids(kChildren) {
        root.initRootHeap(arena.base + 5, size - 5);  // misaligned start exercises the rounding
    }
    Arena arena;
    JASHeap root;
    std::vector<JASHeap> kids;
};

}  // namespace

// Runs the same random sequence of alloc / allocTail / free on a tree driven by the current
// JASHeap::alloc() and on one driven by OriginalAlloc(), and compares everything observable after
// every step. About a fifth of the successful allocations in this sequence go through the
// gap-search path. The seeds are fixed, so a failure is reproducible.
TEST(JASHeap, AllocMatchesTheOriginalAlgorithmOnRandomOperations) {
    for (unsigned seed = 1; seed <= 400; seed++) {
        std::mt19937 rng(seed);
        const size_t size = 4096 + (rng() % 8) * 8192;
        Universe current(size), original(size);

        for (int step = 0; step < 150; step++) {
            const int op = rng() % 10;
            const int i = rng() % Universe::kChildren;
            const u32 want = 1 + rng() % (rng() % 4 == 0 ? size : 6000);

            bool got = false, expected = false;
            if (op < 6) {
                got = current.kids[i].alloc(&current.root, want);
                expected = OriginalAlloc(&original.kids[i], &original.root, want);
            } else if (op < 8) {
                got = current.kids[i].allocTail(&current.root, want);
                expected = original.kids[i].allocTail(&original.root, want);
            } else if (current.kids[i].isAllocated()) {
                got = current.kids[i].free();
                expected = original.kids[i].free();
            }

            const std::string where = "seed " + std::to_string(seed) + ", step " +
                                      std::to_string(step) + ", op " + std::to_string(op) +
                                      ", size " + std::to_string(want);
            ASSERT_EQ(got, expected) << where;
            for (int k = 0; k < Universe::kChildren; k++) {
                const JASHeap& a = current.kids[k];
                const JASHeap& b = original.kids[k];
                ASSERT_EQ(a.isAllocated(), b.isAllocated()) << where << ", child " << k;
                if (a.isAllocated()) {
                    ASSERT_EQ(OffsetIn(current.root, a), OffsetIn(original.root, b))
                        << where << ", child " << k;
                    ASSERT_EQ(a.getSize(), b.getSize()) << where << ", child " << k;
                }
            }
            ASSERT_EQ(current.root.getFreeSize(), original.root.getFreeSize()) << where;
            ASSERT_EQ(current.root.getTotalFreeSize(), original.root.getTotalFreeSize()) << where;
        }
    }
}
