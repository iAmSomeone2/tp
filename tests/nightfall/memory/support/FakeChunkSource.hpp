#pragma once

#include <cstddef>
#include <map>
#include <vector>
#include <nightfall/memory/arena.hpp>

/**
 * A `ChunkSource` for tests, backed by the global aligned `operator new`. It records each chunk's
 * range and owner, and can refuse allocations or decline to answer `owner_of`.
 */
class FakeChunkSource : public nf::ChunkSource {
    struct Allocation {
        std::size_t size;
        std::size_t alignment;
        const void* owner;
    };

    /** Live chunks, keyed by start address. */
    std::map<const std::byte*, Allocation> m_allocations;

public:
    /** When false, `try_allocate` returns null. */
    bool m_accept_allocations = true;
    /** When false, `owner_of` returns `std::nullopt`. */
    bool m_answer_owner_of = true;
    /** When false, `shrink_in_place` returns false and changes nothing. */
    bool m_accept_shrink = true;

    struct Chunk {
        const std::byte* start;
        std::size_t size;
        std::size_t alignment;
        const void* owner;
    };

    FakeChunkSource() = default;
    ~FakeChunkSource() override;

    FakeChunkSource(const FakeChunkSource&) = delete;
    FakeChunkSource& operator=(const FakeChunkSource&) = delete;
    FakeChunkSource(FakeChunkSource&&) = delete;
    FakeChunkSource& operator=(FakeChunkSource&&) = delete;

    [[nodiscard]] void* try_allocate(std::size_t size, std::size_t alignment,
                                     const void* owner) override;
    void deallocate(void* ptr, std::size_t size, std::size_t alignment) override;
    bool shrink_in_place(const void* ptr, std::size_t old_size, std::size_t new_size) override;
    [[nodiscard]] std::optional<const void*> owner_of(const void* ptr) const override;

    /** Number of chunks handed out and not yet given back. */
    [[nodiscard]] std::size_t live() const { return m_allocations.size(); }
    /** Every live chunk, in address order. */
    [[nodiscard]] std::vector<Chunk> chunks() const;
    /** Current size of the chunk starting at `ptr`, or 0 if there is none. */
    [[nodiscard]] std::size_t size_of(const void* ptr) const;
};
