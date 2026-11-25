//
// Created by Brenden Davidson on 11/23/26.
//

#include "nightfall/memory/block_pool.hpp"

#include <algorithm>
#include <cassert>
#include <exception>

using namespace nf;

[[nodiscard]] std::optional<std::size_t> nf::into_block_size(const std::size_t size) noexcept {
    // Create a bitmask from `k_block_size`
    constexpr std::size_t mask = k_block_size - 1;

    // Ensure `size` is within bounds
    if (size > std::numeric_limits<std::size_t>::max() - mask) {
        // block size overflow
        return std::nullopt;
    }

    // Use the mask to round size up
    return (size + mask) & ~mask;
}

BlockPool::OwnerRegistry::OwnerRegistry() noexcept {
    // Reserve enough space for 4KiB worth of owners on 32-bit hosts
    m_owners.reserve(1024);
    m_free_ids.reserve(20);  // Very few IDs are likely to be reused
}

std::size_t BlockPool::OwnerRegistry::high_water_mark() const noexcept {
    return m_high_water_mark;
}

std::optional<const void*> BlockPool::OwnerRegistry::operator[](OwnerId id) const noexcept {
    if (const auto count = m_owners.size();
        id == OwnerId::None || static_cast<std::size_t>(id) > count)
    {
        return std::nullopt;
    }
    return m_owners[static_cast<std::size_t>(id) - 1];
}

std::optional<OwnerId> BlockPool::OwnerRegistry::register_owner(const void* owner) noexcept {
    assert(owner != nullptr);

    // Use a previously-freed ID if one is present
    if (!m_free_ids.empty()) {
        const auto id = m_free_ids.back();
        m_free_ids.pop_back();
        m_owners[static_cast<std::size_t>(id) - 1] = owner;
        return id;
    }

    if (m_owners.size() >= static_cast<std::size_t>(OwnerId::Max)) {
        // Max capacity reached
        return std::nullopt;
    }

    // Allocate a new ID
    m_owners.push_back(owner);
    if (m_owners.size() > m_high_water_mark) {
        m_high_water_mark = m_owners.size();
    }
    return static_cast<OwnerId>(m_owners.size());
}

void BlockPool::OwnerRegistry::unregister_owner(const OwnerId id) noexcept {
    if (id == OwnerId::None || static_cast<std::size_t>(id) > m_owners.size()) {
        return;
    }

    auto& registered_owner = m_owners[static_cast<std::size_t>(id) - 1];
    if (registered_owner == nullptr) {
        // ID has already been freed
        return;
    }
    // Simply set this ID to nullptr until a new owner takes this ID
    registered_owner = nullptr;
    m_free_ids.push_back(id);
}

BlockPool::BlockPool(AllocationSource& source, const std::size_t block_count) noexcept
    : m_source(source),
      m_free_blocks(block_count) {
    const auto byte_count = block_count * k_block_size;
    const auto allocation = m_source.try_allocate(byte_count, alignof(std::max_align_t), nullptr);
    assert(allocation != nullptr);
    m_storage = std::span(static_cast<std::byte*>(allocation), byte_count);
    m_blocks.resize(block_count, OwnerId::None);
}

BlockPool::~BlockPool() noexcept {
    m_source.deallocate(m_storage.data(), m_storage.size(), alignof(std::max_align_t));
}

std::size_t BlockPool::capacity() const noexcept {
    return m_free_blocks + m_used_blocks;
}

std::size_t BlockPool::free_blocks() const noexcept {
    return m_free_blocks;
}

std::size_t BlockPool::used_blocks() const noexcept {
    return m_used_blocks;
}

bool BlockPool::has_allocations(const OwnerId id) const noexcept {
    return std::ranges::any_of(m_blocks,
                               [id](const OwnerId block_owner) { return id == block_owner; });
}

std::optional<OwnerId> BlockPool::register_owner(const void* owner) noexcept {
    // simply defer to the backing registry
    return m_owner_registry.register_owner(owner);
}

void BlockPool::unregister_owner(const OwnerId id) noexcept {
    // Ensure no blocks are currently owned by the given owner
    assert(!has_allocations(id));
    m_owner_registry.unregister_owner(id);
}
