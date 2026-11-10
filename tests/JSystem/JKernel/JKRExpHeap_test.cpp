// Tests for JKernel/JKRExpHeap.cpp: JKRExpHeap::joinTwoBlocks(), which merges a freed block with
// the next free block when they touch, and panics ("Bad Block") if the first one overlaps the next.
//
// Why this test exists
// --------------------
// joinTwoBlocks used to compute both block addresses as u32. On a 64-bit host that truncates them,
// so when a free block ends below a 4 GiB boundary and the next free block starts above it, the
// truncated "next" address is smaller than the truncated "end" address. The heap looked corrupt
// and the game panicked on an ordinary free(). The addresses are uintptr_t now.
//
// The test maps a 64 KiB heap that straddles a 4 GiB boundary and frees a block below the
// boundary while the remaining free space is above it. It is skipped on 32-bit hosts and when the
// OS will not place the mapping at the requested address.

#include <gtest/gtest.h>

#include <sys/mman.h>

#include <cstdint>
#include <new>

#include "JSystem/JKernel/JKRExpHeap.h"

#include "support/load_time_stubs.hpp"
#include "support/os_stubs.hpp"

namespace {

// JKRExpHeap's constructor is protected. A null parent makes it the system and current heap; the
// test puts the previous ones back straight away so that the global operator new (which allocates
// from the current JKR heap) keeps using the host allocator for gtest's own allocations.
struct TestExpHeap : JKRExpHeap {
    TestExpHeap(void* data, u32 size) : JKRExpHeap(data, size, nullptr, false) {}
};

TEST(JKRExpHeap, FreeBelowA4GiBBoundaryWithFreeSpaceAboveIt) {
    if constexpr (sizeof(void*) < 8) {
        GTEST_SKIP() << "needs a 64-bit address space";
    }
    constexpr uintptr_t kBoundary = uintptr_t{2} << 32;  // 8 GiB
    constexpr size_t kMapSize = 0x10000;
    void* hint = reinterpret_cast<void*>(kBoundary - 0x4000);
    void* map = mmap(hint, kMapSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED || map != hint) {
        if (map != MAP_FAILED) munmap(map, kMapSize);
        GTEST_SKIP() << "could not map memory across the 4 GiB boundary";
    }

    constexpr u32 kHeapObj = (sizeof(TestExpHeap) + 0xF) & ~0xF;
    JKRHeap* const prevSystem = JKRHeap::getSystemHeap();
    JKRHeap* const prevCurrent = JKRHeap::getCurrentHeap();
    auto* heap = new (map) TestExpHeap(static_cast<u8*>(map) + kHeapObj, kMapSize - kHeapObj);
    JKRHeap::setSystemHeap(prevSystem);
    JKRHeap::setCurrentHeap(prevCurrent);
    const s32 total = heap->getTotalFreeSize();

    void* below = heap->alloc(0x1000, 0x10);  // well below the boundary
    void* across = heap->alloc(0x4000, 0x10); // crosses it; the free remainder is above it
    ASSERT_NE(below, nullptr);
    ASSERT_NE(across, nullptr);
    ASSERT_LT(reinterpret_cast<uintptr_t>(below) + 0x1000, kBoundary);
    ASSERT_GT(reinterpret_cast<uintptr_t>(across) + 0x4000, kBoundary);

    const s32 beforeFree = heap->getTotalFreeSize();
    heap->free(below);  // joinTwoBlocks(below) with the next free block above the boundary
    EXPECT_GE(heap->getTotalFreeSize(), beforeFree + 0x1000);

    heap->free(across);  // everything merges back into one block
    EXPECT_EQ(heap->getFreeSize(), total);  // the largest free block is the whole heap again
    EXPECT_EQ(heap->getTotalFreeSize(), total);

    munmap(map, kMapSize);
}

}  // namespace
