// A JKR heap over its own mmap'd buffer, for tests.
//
// The heap is parentless, so the thread that constructs it is its owner thread (the only one that
// may use it as the current heap). A null parent also makes it the system and current heap; the
// previous ones are put back at once, so the global operator new keeps sending gtest's own
// allocations to the host allocator.
//
// The buffer comes from mmap, which on a 64-bit host places it above 4 GiB even when the test is
// not a PIE (malloc's heap then sits low), so pointer truncation in the heaps shows up in tests.
//
// The heap's destructor is not run: ~JKRHeap unlinks the heap from its parent, which it does not
// have. Nothing registers with it outside its buffer, so unmapping the buffer is enough.
#pragma once

#include <sys/mman.h>

#include <cstdint>

#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/JKernel/JKRSolidHeap.h"

// The heap constructors are protected.
struct TestExpHeap : JKRExpHeap {
    TestExpHeap(void* data, u32 size) : JKRExpHeap(data, size, nullptr, false) {}
};
struct TestSolidHeap : JKRSolidHeap {
    TestSolidHeap(void* data, u32 size) : JKRSolidHeap(data, size, nullptr, false) {}
};

template <class Heap = TestExpHeap>
class OwnedHeap {
public:
    // With `at`, the buffer must land exactly there; if it cannot, get() is null.
    explicit OwnedHeap(u32 size = 0x10000, void* at = nullptr) : mSize(size) {
        void* map = mmap(at, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (map == MAP_FAILED || (at != nullptr && map != at)) {
            if (map != MAP_FAILED) {
                munmap(map, size);
            }
            return;
        }
        mBuffer = static_cast<u8*>(map);
        constexpr u32 kHeapObj = (sizeof(Heap) + 0x1F) & ~0x1F;
        JKRHeap* const prevSystem = JKRHeap::getSystemHeap();
        JKRHeap* const prevCurrent = JKRHeap::getCurrentHeap();
        mHeap = new (mBuffer) Heap(mBuffer + kHeapObj, size - kHeapObj);
        JKRHeap::setSystemHeap(prevSystem);
        JKRHeap::setCurrentHeap(prevCurrent);
    }
    ~OwnedHeap() {
        if (mBuffer != nullptr) {
            munmap(mBuffer, mSize);
        }
    }
    OwnedHeap(const OwnedHeap&) = delete;
    OwnedHeap& operator=(const OwnedHeap&) = delete;

    Heap* get() const { return mHeap; }
    Heap* operator->() const { return mHeap; }
    Heap& operator*() const { return *mHeap; }

    bool contains(const void* p) const {
        const auto a = reinterpret_cast<std::uintptr_t>(p);
        return a >= reinterpret_cast<std::uintptr_t>(mHeap->getStartAddr()) &&
               a < reinterpret_cast<std::uintptr_t>(mHeap->getEndAddr());
    }

private:
    u32 mSize;
    u8* mBuffer = nullptr;
    Heap* mHeap = nullptr;
};
