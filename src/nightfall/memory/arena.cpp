#include <nightfall/memory/arena.hpp>
#include <nightfall/memory/block_table.hpp>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>


using namespace nf;

[[nodiscard]] uintptr_t detail::Allocator::get_start() const noexcept {
    return 0;
}

[[nodiscard]] uintptr_t detail::Allocator::get_end() const noexcept {
    return 0;
}

[[nodiscard]] uintptr_t detail::Allocator::get_head() const noexcept {
    return 0;
}

[[nodiscard]] inline size_t detail::Allocator::capacity() const noexcept {
    const auto end = get_end();
    const auto start = get_start();
    assert(end >= start);  // Debug assert that pointers aren't flipped
    return end - start;
}

[[nodiscard]] std::size_t detail::Allocator::used() const noexcept {
    const auto head = get_head();
    const auto start = get_start();
    assert(head >= start);
    return head - start;
}

[[nodiscard]] std::size_t detail::Allocator::available() const noexcept {
    const auto end = get_end();
    const auto head = get_head();
    assert(end >= head);
    return end - head;
}


/**
 * Small wrapper used to align chunk sizes to the nearest multiple of `k_block_size`. In debug
 * builds, this asserts that the chunk size does not overflow `size_t`.
 *
 * @param requested bytes to size the Chunk to
 * @return the actual size rounded up to the nearest multiple of `k_block_size`.
 */
[[nodiscard]] static constexpr size_t ensure_chunk_size(const size_t requested) noexcept {
    const auto chunk_size = into_block_size(requested);
    assert(chunk_size.has_value());  // Debug assertion that the requested size is valid
    return chunk_size.value();
}

[[nodiscard]] inline uintptr_t Chunk::get_start() const noexcept {
    return reinterpret_cast<uintptr_t>(m_storage.get());
}

[[nodiscard]] inline uintptr_t Chunk::get_end() const noexcept {
    return m_end;
}

[[nodiscard]] inline uintptr_t Chunk::get_head() const noexcept {
    return m_head;
}

void* Chunk::do_allocate(const std::size_t size, const std::size_t alignment) {
    auto* ptr = reinterpret_cast<void*>(m_head);
    auto free = available();
    void* aligned_ptr = std::align(alignment, size, ptr, free);
    if (aligned_ptr == nullptr) {
        // Not enough space to allocate with given alignment
        // Contract with engine doesn't allow exceptions.
        std::terminate();
    }

    // Bump 'head' to the end of this allocation and return the pointer
    m_head = reinterpret_cast<uintptr_t>(aligned_ptr) + size;
    return aligned_ptr;
}

void Chunk::do_deallocate(void*, std::size_t, std::size_t) {
    // no-op
}

bool Chunk::do_is_equal(memory_resource const& other) const noexcept {
    return other == *this;
}

Chunk::Chunk(void* allocation, const size_t size) noexcept
    : m_prev(nullptr),
      m_head(reinterpret_cast<uintptr_t>(allocation)),
      m_storage(static_cast<std::byte*>(allocation)) {
    m_end = m_head + size;
}

Chunk::Chunk(void* allocation, const size_t size, std::unique_ptr<Chunk>& head) noexcept
    : m_prev(std::move(head)),
      m_head(reinterpret_cast<uintptr_t>(allocation)),
      m_storage(static_cast<std::byte*>(allocation)) {
    m_end = m_head + size;
}

Chunk::~Chunk() noexcept {}

size_t Arena::allocate_chunk(const size_t requested_size) {
    // Allocate the chunk's memory from our `ChunkSource`
    auto allocation_deleter = [this, requested_size](std::byte* ptr) noexcept {
        this->m_source.deallocate(ptr, requested_size, alignof(std::max_align_t));
    };

    auto* allocation = m_source.try_allocate(requested_size, alignof(std::max_align_t), this);
    assert(allocation != nullptr);
    auto chunk = new Chunk(allocation, requested_size);
}

Arena::Arena(detail::ChunkSource& source, std::size_t first_chunk_size,
             const std::size_t min_chunk_size)
    : m_source(source) {
    // Ensure the chunk sizes are a multiple of `k_block_size`
    first_chunk_size = ensure_chunk_size(first_chunk_size);
    m_min_chunk_size = ensure_chunk_size(min_chunk_size);

    this->allocate_chunk(first_chunk_size);
}

Arena::~Arena() {}
