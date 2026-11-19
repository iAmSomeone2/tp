#include "FakeChunkSource.hpp"

#include <cstdlib>
#include <new>

namespace {
const std::byte* as_bytes(const void* ptr) { return static_cast<const std::byte*>(ptr); }
}  // namespace

FakeChunkSource::~FakeChunkSource() {
    // A test that leaks a chunk fails its own `live()` check; this just returns the memory.
    for (const auto& [ptr, a] : m_allocations) {
        ::operator delete(const_cast<std::byte*>(ptr), std::align_val_t{a.alignment});
    }
}

void* FakeChunkSource::try_allocate(const std::size_t size, const std::size_t alignment,
                                    const void* owner) {
    if (!m_accept_allocations) {
        return nullptr;
    }
    void* ptr = ::operator new(size, std::align_val_t{alignment}, std::nothrow);
    if (ptr != nullptr) {
        m_allocations[as_bytes(ptr)] = Allocation{.size = size, .alignment = alignment, .owner = owner};
    }
    return ptr;
}

void FakeChunkSource::deallocate(void* ptr, std::size_t, const std::size_t alignment) {
    const auto it = m_allocations.find(as_bytes(ptr));
    if (it == m_allocations.end() || it->second.alignment != alignment) {
        std::abort();  // not one of ours, freed twice, or freed with the wrong alignment
    }
    m_allocations.erase(it);
    ::operator delete(ptr, std::align_val_t{alignment});
}

bool FakeChunkSource::shrink_in_place(const void* ptr, const std::size_t old_size,
                                      const std::size_t new_size) {
    const auto it = m_allocations.find(as_bytes(ptr));
    if (it == m_allocations.end() || it->second.size != old_size || new_size > old_size) {
        std::abort();
    }
    if (!m_accept_shrink) {
        return false;
    }
    // The underlying allocation keeps its size; only the recorded range shrinks.
    it->second.size = new_size;
    return true;
}

std::optional<const void*> FakeChunkSource::owner_of(const void* ptr) const {
    if (!m_answer_owner_of) {
        return std::nullopt;
    }
    const std::byte* p = as_bytes(ptr);
    auto it = m_allocations.upper_bound(p);
    if (it == m_allocations.begin()) {
        return nullptr;
    }
    --it;
    return p < it->first + it->second.size ? it->second.owner : nullptr;
}

std::size_t FakeChunkSource::size_of(const void* ptr) const {
    const auto it = m_allocations.find(as_bytes(ptr));
    return it == m_allocations.end() ? 0 : it->second.size;
}

std::vector<FakeChunkSource::Chunk> FakeChunkSource::chunks() const {
    std::vector<Chunk> out;
    for (const auto& [start, a] : m_allocations) {
        out.push_back({start, a.size, a.alignment, a.owner});
    }
    return out;
}
