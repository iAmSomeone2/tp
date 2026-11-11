#pragma once

// start_lifetime_as (C++23, P2590) for reading loaded file data as structs without copying it
// (docs/port/memory-design.md). Uses the standard function where the library has it; the Vita's
// libstdc++ and Apple's libc++ do not, and get the C++20 way of creating objects in place.

#include <cstddef>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>
#include <version>

namespace nf {

namespace detail {

// memmove implicitly creates objects of implicit-lifetime types in its destination, and launder
// gives a pointer to the one at p. memmove writes, so the memory must be writable; at -O0 it is a
// real copy, so call this once per table, not per field. Optimized builds remove it.
template <class T>
T* start_lifetime_as_array_fallback(void* p, std::size_t n) noexcept {
    if (n == 0) {
        return static_cast<T*>(p);
    }
    return std::launder(static_cast<T*>(std::memmove(p, p, sizeof(T) * n)));
}

template <class T>
T* start_lifetime_as_fallback(void* p) noexcept {
    return start_lifetime_as_array_fallback<T>(p, 1);
}

template <class T>
constexpr void check_lifetime_type() {
    static_assert(std::is_trivially_copyable_v<T>, "file data must be trivially copyable");
    static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "use the const overload instead");
}

}  // namespace detail

template <class T>
[[nodiscard]] T* start_lifetime_as(void* p) noexcept {
    detail::check_lifetime_type<T>();
#if defined(__cpp_lib_start_lifetime_as) && __cpp_lib_start_lifetime_as >= 202207L
    return std::start_lifetime_as<T>(p);
#else
    return detail::start_lifetime_as_fallback<T>(p);
#endif
}

template <class T>
[[nodiscard]] const T* start_lifetime_as(const void* p) noexcept {
    detail::check_lifetime_type<T>();
#if defined(__cpp_lib_start_lifetime_as) && __cpp_lib_start_lifetime_as >= 202207L
    return std::start_lifetime_as<T>(p);
#else
    return detail::start_lifetime_as_fallback<T>(const_cast<void*>(p));  // see the note above
#endif
}

template <class T>
[[nodiscard]] T* start_lifetime_as_array(void* p, std::size_t n) noexcept {
    detail::check_lifetime_type<T>();
#if defined(__cpp_lib_start_lifetime_as) && __cpp_lib_start_lifetime_as >= 202207L
    return std::start_lifetime_as_array<T>(p, n);
#else
    return detail::start_lifetime_as_array_fallback<T>(p, n);
#endif
}

template <class T>
[[nodiscard]] const T* start_lifetime_as_array(const void* p, std::size_t n) noexcept {
    detail::check_lifetime_type<T>();
#if defined(__cpp_lib_start_lifetime_as) && __cpp_lib_start_lifetime_as >= 202207L
    return std::start_lifetime_as_array<T>(p, n);
#else
    return detail::start_lifetime_as_array_fallback<T>(const_cast<void*>(p), n);
#endif
}

}  // namespace nf
