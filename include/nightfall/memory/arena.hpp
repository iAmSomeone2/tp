#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <new>
#include <optional>

namespace nf {
namespace detail {
struct Allocator {
    virtual ~Allocator() = default;

protected:
    /**
     * Get the current head address of this allocator
     * @return allocator head pointer address
     */
    [[nodiscard]] virtual inline uintptr_t get_head() const noexcept;

    /**
     * Get the start of this allocator's address range
     * @return
     */
    [[nodiscard]] virtual inline uintptr_t get_start() const noexcept;

    /**
     * Get the end of this allocator's address range
     * @return allocator end address
     */
    [[nodiscard]] virtual inline uintptr_t get_end() const noexcept;

public:
    /**
     * Get this allocator's full capacity
     * @return this allocator's full capacity
     */
    [[nodiscard]] std::size_t capacity() const noexcept;
    /**
     * Get the count of allocated bytes
     * @return number of bytes currently allocated
     */
    [[nodiscard]] std::size_t used() const noexcept;
    /**
     * Get the number of bytes not yet allocated
     * @return number of bytes available for allocations
     */
    [[nodiscard]] std::size_t available() const noexcept;
};

/**
 * Abstract base class used as an interface for structures representing chunks of kernel-allocated
 * memory.
 */
struct ChunkSource {
    virtual ~ChunkSource() = default;

    /**
     * Attempts to allocate a block of memory from this chunk source
     * @param size amount of memory (in bytes) to allocate
     * @param alignment byte multiple to align the allocation to (defaults to `std::max_align_t`)
     * @param owner memory management structure which has ownership of this allocation.
     * @return a pointer to the allocated memory on success; otherwise, `nullptr` on failure.
     */
    [[nodiscard]] virtual void* try_allocate(std::size_t size, std::size_t alignment,
                                             const void* owner) = 0;

    /**
     * Deallocates a block of memory allocated from this chunk source
     * @param ptr pointer to the target allocation
     * @param size byte count of allocation
     * @param alignment the allocation's byte alignment
     */
    virtual void deallocate(void* ptr, std::size_t size, std::size_t alignment) = 0;

    /**
     * Attempts to shrink an allocation from `old_size` to `new_size`
     * @param ptr pointer to the target allocation
     * @param old_size current allocation size
     * @param new_size target allocation size
     * @return `true` if the allocation could be shrunk; otherwise, `false`.
     */
    virtual bool shrink_in_place(const void* /*ptr*/, std::size_t /*old_size*/,
                                 std::size_t /*new_size*/) {
        return false;
    }

    /**
     * Looks up the owner of any address inside a chunk from this source.
     * @param ptr the address to look up
     * @return `std::nullopt` if this source can't tell; `nullptr` if nobody owns `ptr`; otherwise
     * the `owner` passed to `try_allocate`.
     */
    [[nodiscard]] virtual std::optional<const void*> owner_of(const void* /*ptr*/) const {
        return std::nullopt;
    }
};
};  // namespace detail

class Arena;

/**
 * A chunk of allocated memory managed by a nf::Arena. It also serves as a linked
 * list node to other previously-allocated Chunks.
 */
class Chunk : public detail::Allocator, public std::pmr::memory_resource {
    /** Previous ChunkHeader in the chain */
    std::unique_ptr<Chunk> m_prev;
    /** Current end of the allocated area */
    uintptr_t m_head;
    /** Ending address of this Chunk's managed memory */
    uintptr_t m_end;
    /** Pointer value to the starting address of this Chunk's managed memory */
    std::unique_ptr<std::byte> m_storage;

    [[nodiscard]] void* do_allocate(std::size_t, std::size_t) override;

    void do_deallocate(void*, std::size_t, std::size_t) override;

    [[nodiscard]] bool do_is_equal(memory_resource const&) const noexcept override;

protected:
    [[nodiscard]] uintptr_t get_start() const noexcept override;
    [[nodiscard]] uintptr_t get_end() const noexcept override;
    [[nodiscard]] uintptr_t get_head() const noexcept override;

public:
    // Delete copy constructors but leave move construction
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    /**
     * Create a new Chunk given an allocation and its size.
     *
     * @param allocation pointer to memory allocated for this Chunk
     * @param size number of bytes in this allocation
     */
    Chunk(void* allocation, size_t size) noexcept;

    /**
     * Create a new Chunk given an allocation, its size, and a reference to the unique pointer
     * of the current head of the Chunk list.
     *
     * @param allocation pointer to memory allocated for this Chunk
     * @param size number of bytes in this allocation
     * @param head current head pointer of the Chunk to link behind this one.
     */
    Chunk(void* allocation, size_t size, std::unique_ptr<Chunk>& head) noexcept;

    ~Chunk() noexcept override;
};

class Arena final {
    detail::ChunkSource& m_source;
    /**
     * Minimum number of bytes to allocate for a chunk.
     *
     * Default: 1 block
     */
    size_t m_min_chunk_size;

public:
private:
    /** Currently-active Chunk */
    std::unique_ptr<Chunk> m_current_chunk;

    Arena(detail::ChunkSource& source, size_t first_chunk_size, size_t min_chunk_size);

    /**
     * Allocates a Chunk and sets `m_current_chunk` to it.
     * @param requested_size of the new Chunk
     * @return actual size of the new Chunk
     */
    size_t allocate_chunk(size_t requested_size);

public:
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) = delete;
    Arena& operator=(Arena&&) = delete;

    ~Arena();

    /**
     *
     * @param slot where the Arena will live
     * @param source where chunks come from
     * @param first_chunk_size size (in bytes) of the initial chunk allocated by this Arena
     * @param min_chunk_size minimum size (in bytes) of each chunk allocated as-needed
     * @return
     */
    static Arena* create(std::optional<Arena>& slot, detail::ChunkSource& source,
                         size_t first_chunk_size, size_t min_chunk_size);

private:
    /**
     * Bumps all small alignments up to `alignof(std::max_align_t)`.
     *
     * @remark Panics in debug builds if an alignment greater than `alignof(std::max_align_t)` is
     * provided which is not a power of 2
     *
     * @param alignment allocation byte alignment
     * @return normalized alignment value
     */
    [[nodiscard]] static constexpr std::size_t
    normalize_alignment(const std::size_t alignment) noexcept {
        // Bump all small alignments up to std::max_align_t
        if (constexpr std::size_t min_alignment = alignof(std::max_align_t);
            alignment <= min_alignment)
        {
            return min_alignment;
        }
        // Ensure all other alignments are powers of 2 in debug builds
        assert(std::has_single_bit(alignment));
        return alignment;
    }
};
}  // namespace nf
