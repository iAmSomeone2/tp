// Tests for nf::Arena (docs/port/memory-design.md, step 4 phase B1).
//
// Arenas are made with `Arena::create(slot, source, first_chunk, growth)`, which constructs the
// arena in the caller's `std::optional<Arena>` (so the first chunk is tagged with its final
// address) and returns null, leaving the slot empty, if the first chunk can't be had.
//
// They assume as little as possible about the arena's chunk header: chunk sizes are checked as
// multiples of the block size and against bounds, never against an exact header size. `used()`
// is taken to count the bytes handed out (with alignment padding, without chunk headers), and
// `reserved()` the bytes taken from the source.

#include "support/FakeChunkSource.hpp"

#include <nightfall/memory/arena.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory_resource>
#include <optional>
#include <vector>

namespace {

using nf::Arena;
using nf::k_block_size;

constexpr std::size_t kMaxAlign = alignof(std::max_align_t);

bool is_aligned(const void* p, std::size_t align) {
    return reinterpret_cast<std::uintptr_t>(p) % align == 0;
}

// The fake's chunk holding `p`, if any.
std::optional<FakeChunkSource::Chunk> chunk_holding(const FakeChunkSource& source, const void* p) {
    const auto* b = static_cast<const std::byte*>(p);
    for (const auto& c : source.chunks()) {
        if (b >= c.start && b < c.start + c.size) {
            return c;
        }
    }
    return std::nullopt;
}

// Allocates `n`-byte blocks until the arena has at least `chunks` chunks.
std::vector<void*> fill_to_chunks(Arena& arena, std::size_t chunks, std::size_t n = 64) {
    std::vector<void*> out;
    while (arena.chunk_count() < chunks) {
        void* p = arena.try_allocate(n);
        if (p == nullptr) {
            ADD_FAILURE() << "try_allocate failed while filling";
            break;
        }
        out.push_back(p);
    }
    return out;
}

// --- Construction -------------------------------------------------------------------------------

TEST(Arena, TakesItsFirstChunkAtConstruction) {
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, 3 * k_block_size, k_block_size), nullptr);
    Arena& arena = *arena_slot;

    ASSERT_EQ(source.live(), 1u);
    const auto chunk = source.chunks().front();
    EXPECT_EQ(chunk.owner, &arena);
    EXPECT_GE(chunk.size, 3 * k_block_size);
    EXPECT_EQ(chunk.size % k_block_size, 0u);
    EXPECT_EQ(arena.chunk_count(), 1u);
    EXPECT_EQ(arena.reserved(), chunk.size);
    EXPECT_EQ(arena.used(), 0u);
    EXPECT_GT(arena.available(), 0u);
}

TEST(Arena, CreateReturnsTheArenaInTheSlot) {
    FakeChunkSource source;
    std::optional<Arena> slot;
    Arena* arena = Arena::create(slot, source, k_block_size, k_block_size);

    ASSERT_TRUE(slot.has_value());
    EXPECT_EQ(arena, &*slot);
    EXPECT_EQ(source.chunks().front().owner, arena) << "first chunk tagged with the final address";
}

TEST(Arena, CreateFailsAndTakesNothingWhenTheSourceRefuses) {
    FakeChunkSource source;
    source.m_accept_allocations = false;
    std::optional<Arena> slot;

    EXPECT_EQ(Arena::create(slot, source, k_block_size, k_block_size), nullptr);
    EXPECT_FALSE(slot.has_value());
    EXPECT_EQ(source.live(), 0u);
}

TEST(Arena, CreateFailsWhenTheFirstChunkOverflows) {
    FakeChunkSource source;
    std::optional<Arena> slot;
    constexpr std::size_t max = std::numeric_limits<std::size_t>::max();

    EXPECT_EQ(Arena::create(slot, source, max, k_block_size), nullptr);
    EXPECT_FALSE(slot.has_value());
    EXPECT_EQ(Arena::create(slot, source, max - k_block_size + 2, k_block_size), nullptr);
    EXPECT_FALSE(slot.has_value());
    EXPECT_EQ(source.live(), 0u);
}

TEST(Arena, CreateReplacesAnArenaAlreadyInTheSlot) {
    FakeChunkSource source;
    std::optional<Arena> slot;
    ASSERT_NE(Arena::create(slot, source, k_block_size, k_block_size), nullptr);
    fill_to_chunks(*slot, 3);

    ASSERT_NE(Arena::create(slot, source, k_block_size, k_block_size), nullptr);
    EXPECT_EQ(source.live(), 1u) << "the old arena's chunks were not given back";
    EXPECT_EQ(slot->chunk_count(), 1u);
}

TEST(Arena, RequestsChunksAtMaxAlign) {
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, k_block_size, k_block_size), nullptr);
    Arena& arena = *arena_slot;
    fill_to_chunks(arena, 3);
    (void)arena.try_allocate(64, 4 * k_block_size);  // over-aligned: padded inside the chunk

    for (const auto& c : source.chunks()) {
        EXPECT_EQ(c.alignment, kMaxAlign);
    }
}

// --- Growth -------------------------------------------------------------------------------------

TEST(Arena, RoundsChunkSizesToBlocks) {
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, 100, 100), nullptr);
    Arena& arena = *arena_slot;  // hints that aren't block multiples
    fill_to_chunks(arena, 4, 200);

    for (const auto& c : source.chunks()) {
        EXPECT_EQ(c.size % k_block_size, 0u) << "chunk of " << c.size << " bytes";
        EXPECT_GE(c.size, k_block_size);
    }
}

TEST(Arena, GrowsAcrossChunksWithoutOverlap) {
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, k_block_size, k_block_size), nullptr);
    Arena& arena = *arena_slot;

    constexpr std::size_t n = 64;
    std::vector<void*> blocks;
    for (int i = 0; i < 1000; ++i) {
        void* p = arena.try_allocate(n);
        ASSERT_NE(p, nullptr) << "allocation " << i;
        blocks.push_back(p);
    }
    EXPECT_GT(arena.chunk_count(), 1u);
    EXPECT_EQ(source.live(), arena.chunk_count());

    // Every block lies wholly inside a chunk owned by the arena...
    for (void* p : blocks) {
        const auto c = chunk_holding(source, p);
        ASSERT_TRUE(c.has_value());
        EXPECT_EQ(c->owner, &arena);
        EXPECT_LE(static_cast<std::byte*>(p) + n, c->start + c->size);
    }
    // ...and no two overlap.
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        std::memset(blocks[i], static_cast<int>(i & 0xFF), n);
    }
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        const auto* b = static_cast<const unsigned char*>(blocks[i]);
        EXPECT_TRUE(std::all_of(b, b + n, [&](unsigned char x) { return x == (i & 0xFF); }))
            << "block " << i << " was overwritten";
    }
}

TEST(Arena, StatisticsFollowAllocations) {
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, 4 * k_block_size, k_block_size), nullptr);
    Arena& arena = *arena_slot;

    const std::size_t available = arena.available();
    (void)arena.try_allocate(100);
    EXPECT_GE(arena.used(), 100u);
    EXPECT_EQ(arena.available(), available - arena.used());
    EXPECT_LE(arena.used(), arena.reserved());

    fill_to_chunks(arena, 3);
    std::size_t reserved = 0;
    for (const auto& c : source.chunks()) {
        reserved += c.size;
    }
    EXPECT_EQ(arena.reserved(), reserved);
    EXPECT_LE(arena.used(), arena.reserved());
}

// --- Alignment ----------------------------------------------------------------------------------

class ArenaAlignment : public ::testing::TestWithParam<std::size_t> {};

TEST_P(ArenaAlignment, IsHonouredInEveryChunk) {
    const std::size_t align = GetParam();
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, k_block_size, k_block_size), nullptr);
    Arena& arena = *arena_slot;

    for (int i = 0; i < 40; ++i) {
        void* p = arena.try_allocate(200, align);
        ASSERT_NE(p, nullptr) << "allocation " << i;
        EXPECT_TRUE(is_aligned(p, std::max(align, kMaxAlign))) << p;
        const auto c = chunk_holding(source, p);
        ASSERT_TRUE(c.has_value());
        EXPECT_LE(static_cast<std::byte*>(p) + 200, c->start + c->size);
    }
    EXPECT_GT(arena.chunk_count(), 1u);
}

INSTANTIATE_TEST_SUITE_P(Arena, ArenaAlignment,
                         ::testing::Values(1, 2, 4, 8, 16, 32, 64, 128, 256, 1024, 4096, 8192));

// --- Oversized requests -------------------------------------------------------------------------

TEST(Arena, OversizedRequestGetsItsOwnChunkWithoutWastingTheCurrentOne) {
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, k_block_size, k_block_size), nullptr);
    Arena& arena = *arena_slot;

    void* first = arena.try_allocate(64);
    ASSERT_NE(first, nullptr);
    const std::size_t available = arena.available();

    constexpr std::size_t big = 5 * k_block_size;
    void* p = arena.try_allocate(big);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(arena.chunk_count(), 2u);
    EXPECT_EQ(arena.available(), available) << "the current chunk's free space was given up";

    // Its chunk is just big enough: the request plus at most one block for header and rounding.
    const auto c = chunk_holding(source, p);
    ASSERT_TRUE(c.has_value());
    EXPECT_GE(c->size, big);
    EXPECT_LE(c->size, big + k_block_size);
    EXPECT_EQ(c->size % k_block_size, 0u);

    // The next small request still comes from the first chunk.
    void* next = arena.try_allocate(64);
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(chunk_holding(source, next)->start, chunk_holding(source, first)->start);
}

// --- release and destruction --------------------------------------------------------------------

TEST(Arena, ReleaseKeepsOnlyTheFirstChunk) {
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, k_block_size, k_block_size), nullptr);
    Arena& arena = *arena_slot;

    void* first = arena.try_allocate(64);
    ASSERT_NE(first, nullptr);
    const auto* first_chunk = chunk_holding(source, first)->start;
    fill_to_chunks(arena, 3);
    (void)arena.try_allocate(5 * k_block_size);  // and an oversized chunk

    arena.release();

    EXPECT_EQ(arena.chunk_count(), 1u);
    ASSERT_EQ(source.live(), 1u);
    EXPECT_EQ(source.chunks().front().start, first_chunk);
    EXPECT_EQ(arena.used(), 0u);
    EXPECT_EQ(arena.reserved(), source.chunks().front().size);
    EXPECT_EQ(arena.try_allocate(64), first) << "release didn't rewind the first chunk";
}

TEST(Arena, DestructorGivesBackEveryChunk) {
    FakeChunkSource source;
    {
        std::optional<Arena> arena_slot;
        ASSERT_NE(Arena::create(arena_slot, source, k_block_size, k_block_size), nullptr);
        Arena& arena = *arena_slot;
        fill_to_chunks(arena, 4);
        (void)arena.try_allocate(5 * k_block_size);
        ASSERT_GT(source.live(), 1u);
    }
    EXPECT_EQ(source.live(), 0u);
}

TEST(Arena, DestructorAfterReleaseGivesBackTheFirstChunk) {
    FakeChunkSource source;
    {
        std::optional<Arena> arena_slot;
        ASSERT_NE(Arena::create(arena_slot, source, k_block_size, k_block_size), nullptr);
        Arena& arena = *arena_slot;
        fill_to_chunks(arena, 3);
        arena.release();
    }
    EXPECT_EQ(source.live(), 0u);
}

// --- shrink_to_fit ------------------------------------------------------------------------------

TEST(Arena, ShrinkToFitGivesBackWholeTrailingBlocks) {
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, 4 * k_block_size, k_block_size), nullptr);
    Arena& arena = *arena_slot;
    void* p = arena.try_allocate(64);
    ASSERT_NE(p, nullptr);

    const std::size_t before = source.chunks().front().size;
    const std::size_t released = arena.shrink_to_fit();
    const std::size_t after = source.chunks().front().size;

    EXPECT_EQ(released, before - after);
    EXPECT_EQ(after, k_block_size) << "header and 64 bytes fit in one block";
    EXPECT_EQ(arena.reserved(), after);
    EXPECT_LT(arena.available(), k_block_size);
    EXPECT_TRUE(arena.contains(p));

    // The arena still works and grows afterwards.
    EXPECT_NE(arena.try_allocate(k_block_size), nullptr);
}

TEST(Arena, ShrinkToFitOnlyTouchesTheCurrentChunk) {
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, k_block_size, 4 * k_block_size), nullptr);
    Arena& arena = *arena_slot;
    void* first = arena.try_allocate(64);
    const auto blocks = fill_to_chunks(arena, 2);  // the second chunk is >= 4 blocks, nearly empty
    ASSERT_NE(first, nullptr);
    ASSERT_FALSE(blocks.empty());

    const auto first_before = *chunk_holding(source, first);
    const auto current_before = *chunk_holding(source, blocks.back());
    ASSERT_GE(current_before.size, 4 * k_block_size);

    const std::size_t released = arena.shrink_to_fit();

    EXPECT_EQ(chunk_holding(source, first)->size, first_before.size);
    EXPECT_EQ(chunk_holding(source, blocks.back())->size, k_block_size);
    EXPECT_EQ(released, current_before.size - k_block_size);
}

TEST(Arena, ShrinkToFitWithARefusingSourceReleasesNothing) {
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, 4 * k_block_size, k_block_size), nullptr);
    Arena& arena = *arena_slot;
    (void)arena.try_allocate(64);
    source.m_accept_shrink = false;

    const std::size_t reserved = arena.reserved();
    EXPECT_EQ(arena.shrink_to_fit(), 0u);
    EXPECT_EQ(arena.reserved(), reserved);
    EXPECT_EQ(source.chunks().front().size, 4 * k_block_size);
}

// --- contains -----------------------------------------------------------------------------------

// true: the source answers owner_of; false: the arena walks its chunk list.
class ArenaContains : public ::testing::TestWithParam<bool> {};

TEST_P(ArenaContains, IsCorrectForEveryChunk) {
    FakeChunkSource source;
    source.m_answer_owner_of = GetParam();
    std::optional<Arena> a_slot;
    ASSERT_NE(Arena::create(a_slot, source, k_block_size, k_block_size), nullptr);
    Arena& a = *a_slot;
    std::optional<Arena> b_slot;
    ASSERT_NE(Arena::create(b_slot, source, k_block_size, k_block_size), nullptr);
    Arena& b = *b_slot;

    constexpr std::size_t n = 64;
    auto blocks = fill_to_chunks(a, 3, n);
    blocks.push_back(a.try_allocate(5 * k_block_size));
    ASSERT_NE(blocks.back(), nullptr);
    void* theirs = b.try_allocate(n);
    ASSERT_NE(theirs, nullptr);

    for (void* p : blocks) {
        EXPECT_TRUE(a.contains(p));
        EXPECT_TRUE(a.contains(static_cast<std::byte*>(p) + n - 1)) << "last byte";
        EXPECT_FALSE(b.contains(p));
    }
    EXPECT_TRUE(b.contains(theirs));
    EXPECT_FALSE(a.contains(theirs));

    int local = 0;
    EXPECT_FALSE(a.contains(&local));
    EXPECT_FALSE(a.contains(nullptr));
}

TEST_P(ArenaContains, IsFalseForReleasedChunks) {
    FakeChunkSource source;
    source.m_answer_owner_of = GetParam();
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, k_block_size, k_block_size), nullptr);
    Arena& arena = *arena_slot;

    fill_to_chunks(arena, 2);
    void* later = arena.try_allocate(5 * k_block_size);  // in a chunk that release gives back
    ASSERT_NE(later, nullptr);
    arena.release();

    EXPECT_FALSE(arena.contains(later));
}

INSTANTIATE_TEST_SUITE_P(Arena, ArenaContains, ::testing::Bool(),
                         [](const auto& info) { return info.param ? "ThroughSource" : "ChunkWalk"; });

// --- Failure ------------------------------------------------------------------------------------

TEST(Arena, RefusingSourceMakesTryAllocateFailAndLeavesTheArenaUnchanged) {
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, k_block_size, k_block_size), nullptr);
    Arena& arena = *arena_slot;
    source.m_accept_allocations = false;

    // Use up the first chunk.
    for (int i = 0; i < 1000 && arena.try_allocate(64) != nullptr; ++i) {
    }

    const std::size_t chunks = arena.chunk_count();
    const std::size_t used = arena.used();
    const std::size_t reserved = arena.reserved();
    const std::size_t available = arena.available();

    EXPECT_EQ(arena.try_allocate(64), nullptr);
    EXPECT_EQ(arena.try_allocate(5 * k_block_size), nullptr);
    EXPECT_EQ(arena.chunk_count(), chunks);
    EXPECT_EQ(arena.used(), used);
    EXPECT_EQ(arena.reserved(), reserved);
    EXPECT_EQ(arena.available(), available);
    EXPECT_EQ(source.live(), 1u);

    source.m_accept_allocations = true;
    EXPECT_NE(arena.try_allocate(64), nullptr) << "the arena recovers once the source accepts";
}

TEST(Arena, OverflowingRequestsFail) {
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, k_block_size, k_block_size), nullptr);
    Arena& arena = *arena_slot;
    constexpr std::size_t max = std::numeric_limits<std::size_t>::max();

    // Size plus padding (or plus header, or rounded to blocks) wraps around.
    EXPECT_EQ(arena.try_allocate(max), nullptr);
    EXPECT_EQ(arena.try_allocate(max - 8, 64), nullptr);
    EXPECT_EQ(arena.try_allocate(max - k_block_size + 1), nullptr);
    EXPECT_EQ(arena.try_allocate(max / 2 + 1, k_block_size), nullptr);

    EXPECT_EQ(arena.chunk_count(), 1u);
    EXPECT_EQ(arena.used(), 0u);
    EXPECT_EQ(source.live(), 1u);
}

TEST(ArenaDeathTest, AllocateAbortsWhenTheSourceRefuses) {
    EXPECT_DEATH(
        {
            FakeChunkSource source;
            std::optional<Arena> arena_slot;
            ASSERT_NE(Arena::create(arena_slot, source, k_block_size, k_block_size), nullptr);
            Arena& arena = *arena_slot;
            source.m_accept_allocations = false;
            (void)arena.allocate(2 * k_block_size);
        },
        "");
}

// --- memory_resource ----------------------------------------------------------------------------

TEST(Arena, DeallocateDoesNothing) {
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, k_block_size, k_block_size), nullptr);
    Arena& arena = *arena_slot;

    void* p = arena.allocate(64);
    const std::size_t used = arena.used();
    arena.deallocate(p, 64);
    EXPECT_EQ(arena.used(), used);
    EXPECT_NE(arena.allocate(64), p) << "memory was reused after deallocate";
}

TEST(Arena, IsEqualIsIdentity) {
    FakeChunkSource source;
    std::optional<Arena> a_slot;
    ASSERT_NE(Arena::create(a_slot, source, k_block_size, k_block_size), nullptr);
    Arena& a = *a_slot;
    std::optional<Arena> b_slot;
    ASSERT_NE(Arena::create(b_slot, source, k_block_size, k_block_size), nullptr);
    Arena& b = *b_slot;

    EXPECT_TRUE(a.is_equal(a));
    EXPECT_FALSE(a.is_equal(b));
}

TEST(Arena, BacksAPmrVectorAcrossChunks) {
    FakeChunkSource source;
    std::optional<Arena> arena_slot;
    ASSERT_NE(Arena::create(arena_slot, source, k_block_size, k_block_size), nullptr);
    Arena& arena = *arena_slot;

    std::pmr::vector<int> v(&arena);
    for (int i = 0; i < 10000; ++i) {
        v.push_back(i);
    }
    EXPECT_GT(arena.chunk_count(), 1u);
    EXPECT_TRUE(arena.contains(v.data()));
    for (int i = 0; i < 10000; ++i) {
        ASSERT_EQ(v[i], i);
    }
}

}  // namespace
