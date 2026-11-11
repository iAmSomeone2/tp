#pragma once

// Owning pointers to memory from a std::pmr::memory_resource (docs/port/memory-design.md).

#include <memory>
#include <memory_resource>
#include <new>
#include <type_traits>
#include <utility>

namespace nf {

// Destroys the object and returns its memory to the resource it came from, with the size and
// alignment it was allocated with. It is templated on T, so a Box<Derived> does not convert to a
// Box<Base>: deallocating through the base would pass the base's size, which a counting or arena
// resource gets wrong.
template <class T>
struct ResourceDelete {
    static_assert(!std::is_array_v<T>, "nf::Box holds one object; arrays need nf::BoxArray");

    std::pmr::memory_resource* resource = nullptr;

    void operator()(T* p) const noexcept {
        std::destroy_at(p);
        resource->deallocate(p, sizeof(T), alignof(T));
    }
};

// Box<T, A> in Rust: an owning pointer that remembers its resource.
template <class T>
using Box = std::unique_ptr<T, ResourceDelete<T>>;

// Box::new_in: allocate from `resource` and construct T in one step. With -fno-exceptions (the
// engine's setting) a failed allocation ends the program, so the result is never empty.
template <class T, class... Args>
[[nodiscard]] Box<T> make_box(std::pmr::memory_resource& resource, Args&&... args) {
    void* memory = resource.allocate(sizeof(T), alignof(T));
    return Box<T>(::new (memory) T(std::forward<Args>(args)...), ResourceDelete<T>{&resource});
}

}  // namespace nf
