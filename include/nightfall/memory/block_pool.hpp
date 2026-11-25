//
// Created by Brenden Davidson on 11/23/26.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <vector>

namespace nf {

struct Allocation;  // Forward decl

/**
 * An AllocationSource implementor is a region of addressable memory from which Allocations can be
 * made.
 *
 * @remark these are not strictly `std::pmr::memory_resource`s because some parts of the engine rely
 * on allocations which may not succeed.
 */
struct AllocationSource {
    virtual ~AllocationSource() = default;

    /**
     * Attempts to allocate a block of memory from this allocation source
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
    virtual bool shrink_in_place(const void* ptr, std::size_t old_size, std::size_t new_size) {
        return false;
    }

    /**
     * Looks up the owner of any address inside a chunk from this source.
     * @param ptr the address to look up
     * @return `std::nullopt` if this source can't tell; `nullptr` if nobody owns `ptr`; otherwise
     * the `owner` passed to `try_allocate`.
     */
    [[nodiscard]] virtual std::optional<const void*> owner_of(const void* ptr) const {
        return std::nullopt;
    }
};

/**
 * Size of a single allocation block
 */
inline constexpr size_t k_block_size = 1024 * 4;  // 4 KiB

/**
 * Rounds the given size up to the next-highest block size
 * @param size value to round
 * @return block-sized value
 */
[[nodiscard]] std::optional<std::size_t> into_block_size(std::size_t size) noexcept;

enum class OwnerId : std::uint16_t {
    None = 0,
    Max = std::numeric_limits<std::uint16_t>::max(),
};

/**
 * FAT-inspired memory allocation management
 *
 * @see https://en.wikipedia.org/wiki/File_Allocation_Table#FAT
 */
class BlockPool {
    // Top-level allocation info

    /**
     * Owner registration helper struct
     */
    struct OwnerRegistry {
    private:
        /**
         * Maximum count of owners registered
         */
        std::size_t m_high_water_mark = 0;
        /**
         * The registry's backing storage
         */
        std::vector<const void*> m_owners;

        /**
         * Stack of IDs which have been unregistered and are free to use
         */
        std::vector<OwnerId> m_free_ids;
    public:
        /**
         * Create a new instance with space reserved for 1024 entries.
         */
        OwnerRegistry() noexcept;

        [[nodiscard]] std::size_t high_water_mark() const noexcept;

        /**
         *
         * @param id OwnerId of the target
         * @return if present, a pointer to the owner registered to the provided ID; otherwise,
         * 'std::nullopt'
         */
        [[nodiscard]] std::optional<const void*> operator[](OwnerId id) const noexcept;

        /**
         * Tries to register a new memory 'owner'
         *
         * @remark OwnerId is backed by u16, and '0' is reserved to mean "no owner", so a maximum of
         * 65,535 owners may be registered.
         *
         * @param owner pointer to the owner to register
         * @return a new OwnerId on success; otherwise, std::nullopt
         */
        [[nodiscard]] std::optional<OwnerId> register_owner(const void* owner) noexcept;

        /**
         * Removes the matching owner from the registry.
         *
         * @remark This simply associates the given OwnerId with 'nullptr'. Using this method does
         * not reclaim a registry slot.
         *
         * @param id OwnerId of owner to remove
         */
        void unregister_owner(OwnerId id) noexcept;
    };

    AllocationSource& m_source;

    /**
     * Count of block available for allocations
     */
    std::size_t m_free_blocks;
    /**
     * Count of blocks currently allocated
     */
    std::size_t m_used_blocks{0};

    /**
     * This table's owner registry
     */
    OwnerRegistry m_owner_registry{};

    /**
     * Ownership record of blocks tracked by this table
     */
    std::vector<OwnerId> m_blocks;

    /**
     * This pool's backing storage allocated from a heap
     */
    std::span<std::byte> m_storage;

public:
    BlockPool(BlockPool&&) = delete;
    BlockPool& operator=(BlockPool&&) = delete;
    BlockPool(const BlockPool&) = delete;
    BlockPool& operator=(const BlockPool&) = delete;

    /**
     * Create a new BlockTable instance with the given capacity.
     *
     * @param source where this BlockPool will reserve its memory from
     * @param block_count count of blocks to reserve
     */
    BlockPool(AllocationSource& source, std::size_t block_count) noexcept;

    ~BlockPool() noexcept;

    /**
     * Get the total block capacity of this table
     * @return total block capacity
     */
    [[nodiscard]] std::size_t capacity() const noexcept;

    /**
     * Get the number of blocks available for allocations
     * @return available blocks
     */
    [[nodiscard]] std::size_t free_blocks() const noexcept;

    /**
     * Get the number of blocks currently allocated
     * @return allocated blocks
     */
    [[nodiscard]] std::size_t used_blocks() const noexcept;

    /**
     * Check whether this OwnerId has any allocations.
     *
     * @remark This scans the block table on each call. Worst case time complexity is O(n) where `n`
     * is the number of blocks this table tracks
     *
     * @param id target to search for
     * @return `true` if
     */
    [[nodiscard]] bool has_allocations(OwnerId id) const noexcept;

    [[nodiscard]] std::optional<OwnerId> register_owner(const void* owner) noexcept;

    void unregister_owner(OwnerId id) noexcept;
};
}  // namespace nf
