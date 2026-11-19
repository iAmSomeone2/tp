#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <new>
#include <optional>

namespace nf {
/**
 * Allocated chunk size
 */
inline constexpr std::size_t k_block_size = 1024 * 4;  // 4 KiB

/**
 * Abstract base class used as an interface for structures representing chunks of kernel-allocated
 * memory.
 */
struct ChunkSource {
    virtual ~ChunkSource() = default;

    /**
     * Attempts to allocate a block of memory from this chunk
     * @param size amount of memory (in bytes) to allocate
     * @param alignment byte multiple to align the allocation to (defaults to `std::max_align_t`)
     * @param owner
     * @return a pointer to the allocated memory on success; otherwise, `nullptr` on failure.
     */
    [[nodiscard]] virtual void* try_allocate(std::size_t size, std::size_t alignment, const void* owner) = 0;

    /**
     * Deallocates a block of memory allocated from this chunk
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
    [[nodiscard]] virtual std::optional<const void*> owner_of(
        const void* /*ptr*/) const {
        return std::nullopt;
    }
};

class Arena final : public std::pmr::memory_resource {
    /** The kernel-allocated memory this Arena is in charge of */

    /**
     * Bumps all small alignments up to `alignof(std::max_align_t)`.
     *
     * @remark Panics in debug builds if an alignment greater than `alignof(std::max_align_t)` is
     * provided which is not a power of 2
     *
     * @param alignment allocation byte alignment
     * @return normalized alignment value
     */
    [[nodiscard]] static constexpr std::size_t normalize_alignment(const std::size_t alignment) noexcept {
        // Bump all small alignments up to std::max_align_t
        if (constexpr std::size_t min_alignment = alignof(std::max_align_t); alignment <= min_alignment)
        {
            return min_alignment;
        }
        // Ensure all other alignments are powers of 2 in debug builds
        assert(std::has_single_bit(alignment));
        return alignment;
    }

public:
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) = delete;
    Arena& operator=(Arena&&) = delete;
};
}  // namespace nf
