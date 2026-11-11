#include <nightfall/memory/lifetime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <version>

namespace {

struct Entry {  // shaped like a baked table entry
    std::uint32_t offset;
    std::uint16_t id;
    std::uint16_t flags;
    float scale;
};

constexpr Entry kEntries[3] = {{0x100, 1, 2, 0.5f}, {0x200, 3, 4, 1.5f}, {0x300, 5, 6, 2.5f}};

// Loaded file bytes: no Entry objects exist here until a lifetime is started.
struct Bytes {
    alignas(Entry) unsigned char data[sizeof kEntries];
    Bytes() { std::memcpy(data, kEntries, sizeof data); }
};

void expect_entries(const Entry* e, int n) {
    for (int i = 0; i < n; i++) {
        EXPECT_EQ(e[i].offset, kEntries[i].offset);
        EXPECT_EQ(e[i].id, kEntries[i].id);
        EXPECT_EQ(e[i].flags, kEntries[i].flags);
        EXPECT_EQ(e[i].scale, kEntries[i].scale);
    }
}

TEST(NfLifetime, FallbackKeepsTheBytes) {
    Bytes b;
    Entry* one = nf::detail::start_lifetime_as_fallback<Entry>(b.data);
    EXPECT_EQ(static_cast<void*>(one), b.data);
    expect_entries(one, 1);
    expect_entries(nf::detail::start_lifetime_as_array_fallback<Entry>(b.data, 3), 3);
    EXPECT_EQ(static_cast<void*>(nf::detail::start_lifetime_as_array_fallback<Entry>(b.data, 0)), b.data);
}

TEST(NfLifetime, PublicFunctionsKeepTheBytes) {
    Bytes b;
    expect_entries(nf::start_lifetime_as<Entry>(b.data), 1);
    expect_entries(nf::start_lifetime_as_array<Entry>(b.data, 3), 3);
    const Bytes& cb = b;
    const Entry* e = nf::start_lifetime_as_array<Entry>(cb.data, 3);
    expect_entries(e, 3);
    expect_entries(nf::start_lifetime_as<Entry>(static_cast<const void*>(cb.data)), 1);
}

TEST(NfLifetime, StandardLibraryPath) {
#if defined(__cpp_lib_start_lifetime_as) && __cpp_lib_start_lifetime_as >= 202207L
    Bytes b;
    expect_entries(std::start_lifetime_as_array<Entry>(b.data, 3), 3);
#else
    GTEST_SKIP() << "this standard library has no std::start_lifetime_as; the fallback is used";
#endif
}

}  // namespace
