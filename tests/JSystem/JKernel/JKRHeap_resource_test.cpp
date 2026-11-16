// Tests for JKRHeap as a std::pmr::memory_resource (docs/port/memory-design.md, step 0).
//
// A JKRExpHeap over a plain buffer stands in for every JKR heap: the resource functions live in
// JKRHeap and forward to alloc()/free(), so the heap type only matters for what alloc() does.
//
// The contract checked here:
//   * allocate() never returns null: a failure ends the program with std::abort(), because the
//     engine builds without exceptions and containers do not check for null.
//   * the result is aligned as asked; alignments of 128 or more are refused (JKRExpHeap keeps
//     the alignment in 7 bits of a block header), as are sizes that do not fit in a u32.
//   * deallocate() gives the memory back to the heap.
//   * is_equal() is identity.

#include <gtest/gtest.h>

#include <csignal>
#include <cstdint>
#include <memory_resource>
#include <new>
#include <type_traits>
#include <vector>

#include "JSystem/JKernel/JKRExpHeap.h"
#include "nightfall/memory/box.hpp"

#include "support/load_time_stubs.hpp"
#include "support/os_stubs.hpp"
#include "support/owned_heap.hpp"

static_assert(std::is_base_of_v<std::pmr::memory_resource, JKRHeap>);

namespace {

constexpr u32 kHeapSize = 0x10000;

TEST(JKRHeapResource, AllocateAndDeallocateGoThroughTheHeap) {
    OwnedHeap<> heap(kHeapSize);
    std::pmr::memory_resource& r = *heap;
    const s32 total = heap->getTotalFreeSize();

    void* p = r.allocate(100);
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(heap.contains(p));
    EXPECT_LT(heap->getTotalFreeSize(), total);

    r.deallocate(p, 100);
    EXPECT_EQ(heap->getTotalFreeSize(), total);
}

TEST(JKRHeapResource, HonoursRequestedAlignment) {
    OwnedHeap<> heap(kHeapSize);
    std::pmr::memory_resource& r = *heap;
    const s32 total = heap->getTotalFreeSize();
    for (std::size_t align = 1; align < 128; align *= 2) {
        void* p = r.allocate(24, align);
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % align, 0u) << "alignment " << align;
        r.deallocate(p, 24, align);
    }
    EXPECT_EQ(heap->getTotalFreeSize(), total);
}

TEST(JKRHeapResource, DefaultAlignmentIsMaxAlign) {
    OwnedHeap<> heap(kHeapSize);
    void* p = static_cast<std::pmr::memory_resource&>(*heap).allocate(24);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % alignof(std::max_align_t), 0u);
}

TEST(JKRHeapResource, IsEqualIsIdentity) {
    OwnedHeap<> a(kHeapSize), b(kHeapSize);
    const std::pmr::memory_resource& ra = *a;
    EXPECT_TRUE(ra.is_equal(*a));
    EXPECT_FALSE(ra.is_equal(*b));
    EXPECT_FALSE(ra.is_equal(*std::pmr::new_delete_resource()));
    EXPECT_TRUE(*a == *a);  // operator== on memory_resource uses is_equal
    EXPECT_FALSE(*a == *b);
}

TEST(JKRHeapResource, PmrVectorGrowsInsideTheHeapAndGivesItAllBack) {
    OwnedHeap<> heap(kHeapSize);
    const s32 total = heap->getTotalFreeSize();
    {
        // Each growth allocates a larger buffer and frees the old one through deallocate().
        std::pmr::vector<int> v(&*heap);
        for (int i = 0; i < 2000; i++) {
            v.push_back(i);
        }
        EXPECT_TRUE(heap.contains(v.data()));
        EXPECT_TRUE(heap.contains(&v.back()));
        for (int i = 0; i < 2000; i++) {
            ASSERT_EQ(v[i], i);
        }
    }
    EXPECT_EQ(heap->getTotalFreeSize(), total);
}

TEST(JKRHeapResource, BoxReturnsMemoryToItsHeap) {
    OwnedHeap<> a(kHeapSize), b(kHeapSize);
    const s32 totalA = a->getTotalFreeSize(), totalB = b->getTotalFreeSize();
    {
        nf::Box<std::uint64_t> x = nf::make_box<std::uint64_t>(*a, 1);
        nf::Box<std::uint64_t> y = nf::make_box<std::uint64_t>(*b, 2);
        EXPECT_TRUE(a.contains(x.get()));
        EXPECT_TRUE(b.contains(y.get()));
    }
    EXPECT_EQ(a->getTotalFreeSize(), totalA);
    EXPECT_EQ(b->getTotalFreeSize(), totalB);
}

// The failures must end the program by std::abort() (SIGABRT), not by a crash further on.
using JKRHeapResourceDeathTest = ::testing::Test;

TEST_F(JKRHeapResourceDeathTest, AbortsWhenTheHeapIsExhausted) {
    OwnedHeap<> heap(kHeapSize);
    EXPECT_EXIT(static_cast<std::pmr::memory_resource&>(*heap).allocate(kHeapSize * 2),
                ::testing::KilledBySignal(SIGABRT), "");
}

TEST_F(JKRHeapResourceDeathTest, AbortsOnASizeThatDoesNotFitInU32) {
    if constexpr (sizeof(std::size_t) <= 4) {
        GTEST_SKIP() << "size_t is 32 bits here";
    }
    OwnedHeap<> heap(kHeapSize);
    // Truncated to u32 this would be a 16-byte request, which the heap could serve.
    const std::size_t huge = (std::size_t{1} << 32) + 16;
    EXPECT_EXIT(static_cast<std::pmr::memory_resource&>(*heap).allocate(huge),
                ::testing::KilledBySignal(SIGABRT), "");
}

TEST_F(JKRHeapResourceDeathTest, AbortsOnAlignmentOf128OrMore) {
    OwnedHeap<> heap(kHeapSize);
    EXPECT_EXIT(static_cast<std::pmr::memory_resource&>(*heap).allocate(16, 128),
                ::testing::KilledBySignal(SIGABRT), "");
}

}  // namespace
