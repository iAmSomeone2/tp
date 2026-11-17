// Tests for finding a heap by address when a heap owns more than one range
// (docs/port/memory-design.md, step 4 phase A).
//
// JKRHeap::find/findAllHeap/findFromRoot decide by JKRHeap::contains(), which a heap may override.
// That is what lets a chunked arena (phase B/C) own memory outside [mStart, mEnd). The stand-in here
// is a solid heap that also claims a second block taken from its parent, the way an arena chunk is.
// Without contains(), a pointer in that block resolves to the parent: operator delete would free a
// non-block in the parent, and a JKRDisposer there would register with the wrong heap.

#include <gtest/gtest.h>

#include <cstdint>
#include <new>

#include "JSystem/JKernel/JKRDisposer.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/JKernel/JKRSolidHeap.h"

#include "support/load_time_stubs.hpp"
#include "support/os_stubs.hpp"
#include "support/owned_heap.hpp"

namespace {

constexpr u32 kRootSize = 0x40000;
constexpr u32 kFirstSize = 0x2000;
constexpr u32 kSecondSize = 0x4000;

// A solid heap that also owns [mSecond, mSecond + mSecondSize).
struct TwoRangeHeap : JKRSolidHeap {
    TwoRangeHeap(void* data, u32 size, JKRHeap* parent, u8* second, u32 secondSize)
        : JKRSolidHeap(data, size, parent, false), mSecond(second), mSecondSize(secondSize) {}

    bool contains(const void* p) const noexcept override {
        return JKRSolidHeap::contains(p) || (mSecond <= p && p < mSecond + mSecondSize);
    }

    u8* mSecond;
    u32 mSecondSize;
};

struct Probe : JKRDisposer {
    explicit Probe(bool* destroyed) : mDestroyed(destroyed) {}
    ~Probe() override { *mDestroyed = true; }
    bool* mDestroyed;
};

// A root exp heap made sRootHeap, with a TwoRangeHeap child whose second range is a separate block
// of the root. Globals are restored afterwards.
class JKRHeapFind : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_NE(mRoot.get(), nullptr);
        mPrevRoot = JKRHeap::sRootHeap;
        mPrevSystem = JKRHeap::getSystemHeap();
        mPrevCurrent = JKRHeap::getCurrentHeap();
        JKRHeap::sRootHeap = mRoot.get();

        constexpr u32 kObj = (sizeof(TwoRangeHeap) + 0x1F) & ~0x1F;
        auto* mem = static_cast<u8*>(mRoot->alloc(kObj + kFirstSize, 0x20));
        // A block between the two ranges keeps them apart.
        mGap = mRoot->alloc(0x100, 0x20);
        mSecond = static_cast<u8*>(mRoot->alloc(kSecondSize, 0x20));
        ASSERT_NE(mem, nullptr);
        ASSERT_NE(mGap, nullptr);
        ASSERT_NE(mSecond, nullptr);
        mHeap = new (mem) TwoRangeHeap(mem + kObj, kFirstSize, mRoot.get(), mSecond, kSecondSize);
    }

    void TearDown() override {
        if (mHeap != nullptr) {
            mHeap->destroy();
        }
        mRoot->free(mSecond);
        mRoot->free(mGap);
        JKRHeap::sRootHeap = mPrevRoot;
        JKRHeap::setSystemHeap(mPrevSystem);
        JKRHeap::setCurrentHeap(mPrevCurrent);
    }

    OwnedHeap<> mRoot{kRootSize};
    TwoRangeHeap* mHeap = nullptr;
    u8* mSecond = nullptr;
    void* mGap = nullptr;

private:
    JKRHeap* mPrevRoot = nullptr;
    JKRHeap* mPrevSystem = nullptr;
    JKRHeap* mPrevCurrent = nullptr;
};

TEST_F(JKRHeapFind, PointerInSecondRangeResolvesToTheChild) {
    EXPECT_EQ(JKRHeap::findFromRoot(mSecond), mHeap);
    EXPECT_EQ(JKRHeap::findFromRoot(mSecond + kSecondSize - 1), mHeap);
    EXPECT_EQ(mRoot->find(mSecond + 0x40), mHeap);
    EXPECT_EQ(mRoot->findAllHeap(mSecond + 0x40), mHeap);
}

TEST_F(JKRHeapFind, OtherPointersResolveAsBefore) {
    EXPECT_EQ(JKRHeap::findFromRoot(mHeap->getStartAddr()), mHeap);
    EXPECT_EQ(JKRHeap::findFromRoot(mGap), mRoot.get());
    EXPECT_EQ(JKRHeap::findFromRoot(mSecond + kSecondSize), mRoot.get());
}

TEST_F(JKRHeapFind, DisposerInSecondRangeIsFreedWithTheChild) {
    bool destroyed = false;
    auto* probe = new (mSecond + 0x40) Probe(&destroyed);
    EXPECT_EQ(probe->mHeap, mHeap);

    mHeap->freeAll();
    EXPECT_TRUE(destroyed);
}

TEST_F(JKRHeapFind, HeapNestedInSecondRangeFindsItsParentAndItsBlocks) {
    // A null parent makes create() look the parent up by address.
    JKRExpHeap* nested = JKRExpHeap::create(mSecond + 0x100, 0x1000, nullptr, false);
    ASSERT_NE(nested, nullptr);
    EXPECT_EQ(nested->getParent(), mHeap);

    void* p = nested->alloc(0x40, 0x10);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(JKRHeap::findFromRoot(p), nested);
    EXPECT_EQ(JKRHeap::findFromRoot(mSecond), mHeap);  // in front of the nested heap

    nested->free(p);
    nested->destroy();
}

}  // namespace
