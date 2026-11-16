// Tests for keeping the current heap on the main thread (docs/port/memory-design.md, step 0).
//
// The current heap is one global, used only by the thread that created the root heap (main01 in
// the game). Worker threads name the heap they allocate from. In builds with assertions, a worker
// that touches the current heap (reads it, changes it, or allocates from it implicitly through the
// global operator new or a null heap argument) ends the program.
//
// The thread that constructs a heap without a parent is the main thread here; the fixture does
// that on gtest's main thread.

#include <gtest/gtest.h>

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <thread>

#include "JSystem/JKernel/JKRExpHeap.h"

#include "support/load_time_stubs.hpp"
#include "support/os_stubs.hpp"

namespace {

constexpr u32 kHeapSize = 0x10000;

struct TestExpHeap : JKRExpHeap {
    TestExpHeap(void* data, u32 size) : JKRExpHeap(data, size, nullptr, false) {}
};

// A root-like ExpHeap over its own buffer. Constructing it records the calling thread as the main
// thread; the previous system and current heaps are put back so gtest keeps using the host
// allocator. Its destructor is not run (~JKRHeap unlinks from a parent it does not have).
class OwnedHeap {
public:
    OwnedHeap() : mBuffer(static_cast<u8*>(std::aligned_alloc(32, kHeapSize))) {
        constexpr u32 kHeapObj = (sizeof(TestExpHeap) + 0x1F) & ~0x1F;
        JKRHeap* const prevSystem = JKRHeap::getSystemHeap();
        JKRHeap* const prevCurrent = JKRHeap::getCurrentHeap();
        mHeap = new (mBuffer) TestExpHeap(mBuffer + kHeapObj, kHeapSize - kHeapObj);
        JKRHeap::setSystemHeap(prevSystem);
        JKRHeap::setCurrentHeap(prevCurrent);
    }
    ~OwnedHeap() { std::free(mBuffer); }
    OwnedHeap(const OwnedHeap&) = delete;
    OwnedHeap& operator=(const OwnedHeap&) = delete;

    JKRExpHeap* get() const { return mHeap; }
    JKRExpHeap* operator->() const { return mHeap; }

    bool contains(const void* p) const {
        auto a = reinterpret_cast<std::uintptr_t>(p);
        return a >= reinterpret_cast<std::uintptr_t>(mHeap->getStartAddr()) &&
               a < reinterpret_cast<std::uintptr_t>(mHeap->getEndAddr());
    }

private:
    u8* mBuffer;
    TestExpHeap* mHeap;
};

// Makes `heap` the root and current heap for the scope (on the main thread), and puts the previous
// ones back. Everything allocated meanwhile through the global operator new comes from `heap`,
// std::thread's own state included; operator delete finds it again through the root heap (a
// pointer no heap under the root owns is passed to std::free).
class CurrentHeapScope {
public:
    explicit CurrentHeapScope(JKRHeap* heap)
        : mPrevRoot(JKRHeap::sRootHeap), mPrevCurrent(JKRHeap::getCurrentHeap()) {
        JKRHeap::sRootHeap = heap;
        JKRHeap::setCurrentHeap(heap);
    }
    ~CurrentHeapScope() {
        JKRHeap::setCurrentHeap(mPrevCurrent);
        JKRHeap::sRootHeap = mPrevRoot;
    }

private:
    JKRHeap* mPrevRoot;
    JKRHeap* mPrevCurrent;
};

template <class F>
void onWorkerThread(F f) {
    std::thread(f).join();
}

TEST(JKRHeapThread, MainThreadAllocatesFromTheCurrentHeap) {
    OwnedHeap heap;
    CurrentHeapScope scope(heap.get());
    EXPECT_EQ(JKRHeap::getCurrentHeap(), heap.get());

    int* p = new int(7);
    EXPECT_TRUE(heap.contains(p));
    delete p;

    void* q = JKRAllocFromHeap(nullptr, 64, 0);  // a null heap means the current heap
    EXPECT_TRUE(heap.contains(q));
    JKRFree(q);
}

TEST(JKRHeapThread, WorkerAllocatesFromANamedHeap) {
    OwnedHeap heap;
    const s32 total = heap->getTotalFreeSize();
    CurrentHeapScope scope(heap.get());  // a current heap exists, but the worker does not use it

    bool inHeap = false;
    onWorkerThread([&] {
        void* a = JKRAllocFromHeap(heap.get(), 64, 0);
        int* b = new (heap.get(), 0) int(1);
        void* c = heap->alloc(32, -32);  // tail, aligned
        inHeap = heap.contains(a) && heap.contains(b) && heap.contains(c);
        heap->free(a);
        heap->free(b);
        heap->free(c);
    });
    EXPECT_TRUE(inHeap);
    EXPECT_EQ(heap->getTotalFreeSize(), total);
}

// A worker touching the current heap is a programming error that assertions catch, so these run
// only in builds that have them (the Debug preset).
class JKRHeapThreadDeathTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef NDEBUG
        GTEST_SKIP() << "assertions are compiled out (NDEBUG)";
#endif
        // The default "fast" style forks without exec, which is unsafe once threads exist.
        GTEST_FLAG_SET(death_test_style, "threadsafe");
    }
};

TEST_F(JKRHeapThreadDeathTest, WorkerImplicitNewAborts) {
    EXPECT_EXIT(
        {
            OwnedHeap heap;
            CurrentHeapScope scope(heap.get());
            onWorkerThread([] { delete new int(1); });
        },
        ::testing::KilledBySignal(SIGABRT), "");
}

TEST_F(JKRHeapThreadDeathTest, WorkerAllocWithANullHeapAborts) {
    EXPECT_EXIT(
        {
            OwnedHeap heap;
            CurrentHeapScope scope(heap.get());
            onWorkerThread([] { JKRAllocFromHeap(nullptr, 64, 0); });
        },
        ::testing::KilledBySignal(SIGABRT), "");
}

TEST_F(JKRHeapThreadDeathTest, WorkerReadingTheCurrentHeapAborts) {
    EXPECT_EXIT(
        {
            OwnedHeap heap;
            CurrentHeapScope scope(heap.get());
            onWorkerThread([] { (void)JKRHeap::getCurrentHeap(); });
        },
        ::testing::KilledBySignal(SIGABRT), "");
}

TEST_F(JKRHeapThreadDeathTest, WorkerSetCurrentHeapAborts) {
    EXPECT_EXIT(
        {
            OwnedHeap heap;
            onWorkerThread([&] { JKRHeap::setCurrentHeap(heap.get()); });
        },
        ::testing::KilledBySignal(SIGABRT), "");
}

TEST_F(JKRHeapThreadDeathTest, WorkerBecomeCurrentHeapAborts) {
    EXPECT_EXIT(
        {
            OwnedHeap heap;
            onWorkerThread([&] { heap->becomeCurrentHeap(); });
        },
        ::testing::KilledBySignal(SIGABRT), "");
}

}  // namespace
