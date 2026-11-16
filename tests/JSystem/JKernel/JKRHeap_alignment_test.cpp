// Tests for the heaps' minimum alignment (docs/port/memory-design.md, step 0): every allocation
// that does not ask for more is aligned to MIN_ALLOC_ALIGN (alignof(std::max_align_t)), because
// host code (SSE, std containers, aligned loads) assumes what malloc gives.
//
// One allocation from a fresh heap proves little: it lands at the start of the heap. JKRExpHeap
// keeps the alignment only if every block header is aligned, the header size is a multiple of the
// minimum and every block size is too, so the ExpHeap tests allocate odd sizes from both ends,
// free some so neighbours join, resize, and mix in aligned allocations (which shift the header and
// keep the offset in its flags), checking every default allocation along the way.

#include <gtest/gtest.h>

#include <cstdint>
#include <new>
#include <vector>

#include "support/load_time_stubs.hpp"
#include "support/os_stubs.hpp"
#include "support/owned_heap.hpp"

namespace {

constexpr u32 kSizes[] = {1, 3, 17, 100, 5, 33, 7, 250, 2, 64, 12, 1000};

std::uintptr_t addr(const void* p) {
    return reinterpret_cast<std::uintptr_t>(p);
}

// Allocates with `alignment` and checks the result is aligned to at least `expected`.
void* allocAligned(JKRHeap* heap, u32 size, int alignment, std::uintptr_t expected) {
    void* p = heap->alloc(size, alignment);
    EXPECT_NE(p, nullptr) << "size " << size << ", alignment " << alignment;
    EXPECT_EQ(addr(p) % expected, 0u)
        << "size " << size << ", alignment " << alignment << ", address " << p;
    return p;
}

// The alignments the engine passes for "don't care": 0 and the original 4, from either end.
constexpr int kDefaultAlignments[] = {0, 4, -4};

void* allocDefault(JKRHeap* heap, u32 size, int i) {
    return allocAligned(heap, size, kDefaultAlignments[i % 3], MIN_ALLOC_ALIGN);
}

TEST(JKRHeapAlignment, MinimumIsMaxAlign) {
    EXPECT_EQ(MIN_ALLOC_ALIGN, alignof(std::max_align_t));
    EXPECT_EQ(sizeof(JKRExpHeap::CMemBlock) % MIN_ALLOC_ALIGN, 0u);
}

TEST(JKRHeapAlignment, ExpHeapDefaultAllocationsStayAlignedThroughFreesAndJoins) {
    OwnedHeap<TestExpHeap> heap;
    const s32 total = heap->getTotalFreeSize();

    std::vector<void*> live;
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < static_cast<int>(std::size(kSizes)); i++) {
            live.push_back(allocDefault(heap.get(), kSizes[i] + round, i + round));
        }
        // Free every other block, so later allocations reuse holes of odd sizes and freed
        // neighbours join.
        for (std::size_t i = round % 2; i < live.size(); i += 2) {
            heap->free(live[i]);
            live[i] = nullptr;
        }
        std::erase(live, nullptr);
    }
    for (void* p : live) {
        heap->free(p);
    }
    EXPECT_EQ(heap->getTotalFreeSize(), total);
}

TEST(JKRHeapAlignment, ExpHeapStaysAlignedAfterResize) {
    OwnedHeap<TestExpHeap> heap;
    void* a = allocDefault(heap.get(), 100, 0);
    void* b = allocDefault(heap.get(), 100, 0);
    void* c = allocDefault(heap.get(), 100, 0);

    // Shrinking splits a free block off the end of `a`; growing `b` into the space freed by `c`
    // splits off the rest.
    EXPECT_GE(heap->resize(a, 13), 13);
    heap->free(c);
    EXPECT_GE(heap->resize(b, 150), 150);

    for (int i = 0; i < static_cast<int>(std::size(kSizes)); i++) {
        allocDefault(heap.get(), kSizes[i], i);
    }
}

TEST(JKRHeapAlignment, ExpHeapAlignedHeadAllocationsKeepTheRestAligned) {
    OwnedHeap<TestExpHeap> heap;
    std::vector<void*> aligned;
    for (int i = 0; i < static_cast<int>(std::size(kSizes)); i++) {
        allocDefault(heap.get(), kSizes[i], 0);  // moves the head to an odd offset
        aligned.push_back(allocAligned(heap.get(), kSizes[i], 32, 32));
        aligned.push_back(allocAligned(heap.get(), kSizes[i], 64, 64));
    }
    for (void* p : aligned) {
        heap->free(p);  // gives back the header offset kept in the flags
    }
    for (int i = 0; i < static_cast<int>(std::size(kSizes)); i++) {
        allocDefault(heap.get(), kSizes[i], i);
    }
}

// This path once built the block pointer from a u32, which crashed with the heap above 4 GiB.
TEST(JKRHeapAlignment, ExpHeapAlignedTailAllocationsKeepTheRestAligned) {
    OwnedHeap<TestExpHeap> heap;
    std::vector<void*> aligned;
    for (int i = 0; i < static_cast<int>(std::size(kSizes)); i++) {
        allocDefault(heap.get(), kSizes[i], 2);  // tail, default alignment
        aligned.push_back(allocAligned(heap.get(), kSizes[i], -32, 32));
        aligned.push_back(allocAligned(heap.get(), kSizes[i], -64, 64));
    }
    for (void* p : aligned) {
        heap->free(p);
    }
    for (int i = 0; i < static_cast<int>(std::size(kSizes)); i++) {
        allocDefault(heap.get(), kSizes[i], i);
    }
}

TEST(JKRHeapAlignment, SolidHeapDefaultAllocationsAreAlignedAtBothEnds) {
    OwnedHeap<TestSolidHeap> heap;
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < static_cast<int>(std::size(kSizes)); i++) {
            allocDefault(heap.get(), kSizes[i] + round, i + round);
        }
    }
}

}  // namespace
