// Tests for JMessage/resource.cpp: looking up a message index from a message ID.
//
// What is under test
// ------------------
// JMessage::TResource::toMessageIndex_messageID() searches the "MID1" block of a message (BMG)
// file. That block is a header followed by a table of **32-bit** message IDs; the position of an
// ID in the table is the message index. Depending on the block's flags the table is sorted
// (binary search) or not (linear search).
//
// Why these tests exist
// ---------------------
// During the 64-bit port the table pointer returned by TParse_TBlock_messageID::getContent()
// became `uintptr_t*`. On a 64-bit host that walks the table 8 bytes at a time and compares
// merged pairs of IDs, so almost every lookup silently failed (3,562 hits where the original
// code found 80,047 in a 120,000-query comparison). Nothing failed to compile, which is why this
// is covered by a test: any change to the element type or stride of the table breaks several of
// the cases below immediately.
//
// Block layout (offsets in bytes), as read by TParse_TBlock_messageID:
//   0x00 u32  block type ('MID1')            0x0A u8   low nibble: form (must be 0)
//   0x04 u32  block size                                 high nibble: non-zero = sorted table
//   0x08 u16  number of IDs                  0x0B u8   form supplement (0..4, see below)
//   0x10 ...  the IDs, one u32 each
//
// Byte order: JSystem reads these fields natively, so the blocks here are built in host byte
// order. Real files are big-endian; when endianness handling is added to the parsers,
// Mid1Block below is the one place that needs to change.
//
// How it is built: the test links the real JMessage code (see tests/JSystem/CMakeLists.txt).

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

#include "JSystem/JMessage/resource.h"

#include "support/load_time_stubs.hpp"

namespace {

constexpr u16 kNotFound = 0xFFFF;

// Builds a MID1 block in memory, in host byte order (see the file comment).
class Mid1Block {
public:
    Mid1Block(const std::vector<u32>& ids, bool sorted, u8 formSupplement)
        : bytes_(kHeaderSize + ids.size() * sizeof(u32) + kPadding, 0) {
        put<u32>(0x00, 0x3144494D);  // 'MID1' as a number; the lookup does not check it
        put<u32>(0x04, static_cast<u32>(kHeaderSize + ids.size() * sizeof(u32)));
        put<u16>(0x08, static_cast<u16>(ids.size()));
        put<u8>(0x0A, sorted ? 0x10 : 0x00);  // form 0 | (sorted flag << 4)
        put<u8>(0x0B, formSupplement);
        if (!ids.empty()) {
            std::memcpy(bytes_.data() + kHeaderSize, ids.data(), ids.size() * sizeof(u32));
        }
    }

    const void* data() const { return bytes_.data(); }

private:
    static constexpr size_t kHeaderSize = 0x10;
    // A few spare bytes so an out-of-stride read stays inside the allocation. A wrong stride
    // would then show up as a wrong answer, not as a crash that hides which test caught it.
    static constexpr size_t kPadding = 64 * sizeof(u32);

    template <typename T>
    void put(size_t offset, T value) {
        std::memcpy(bytes_.data() + offset, &value, sizeof(T));
    }

    std::vector<std::uint8_t> bytes_;
};

// Looks `id`/`upper` up in a block built from `ids` and returns what the code under test says.
u16 Lookup(const std::vector<u32>& ids, bool sorted, u8 supplement, u32 id, u32 upper = 0,
           bool* valid = nullptr) {
    Mid1Block block(ids, sorted, supplement);
    JMessage::TResource resource;
    resource.setData_block_messageID(block.data());
    return resource.toMessageIndex_messageID(id, upper, valid);
}

// Independent oracle: the index of `value` in `ids`, found by plain linear search over u32s.
u16 ExpectedIndex(const std::vector<u32>& ids, u32 value) {
    auto it = std::find(ids.begin(), ids.end(), value);
    return it == ids.end() ? kNotFound : static_cast<u16>(it - ids.begin());
}

}  // namespace

// --- The table element type ------------------------------------------------------------------

// The direct form of the regression: the table is made of 32-bit words, whatever the width of a
// pointer on the host.
TEST(JMessageResource, MessageIdTableEntriesAreFourBytes) {
    using Parse = JMessage::data::TParse_TBlock_messageID;
    static_assert(sizeof(*std::declval<const Parse&>().getContent()) == 4,
                  "MID1 entries are 32-bit words; widening them changes the lookup stride");
    SUCCEED();
}

// --- Sorted tables (binary search) -----------------------------------------------------------

TEST(JMessageResource, SortedTableFindsEveryEntry) {
    const std::vector<u32> ids = {10, 20, 30, 40, 50, 60, 70};
    for (size_t i = 0; i < ids.size(); i++) {
        EXPECT_EQ(Lookup(ids, true, 0, ids[i]), i) << "id " << ids[i];
    }
}

TEST(JMessageResource, SortedTableReportsMissingIds) {
    const std::vector<u32> ids = {10, 20, 30, 40};
    EXPECT_EQ(Lookup(ids, true, 0, 5), kNotFound);    // before the first entry
    EXPECT_EQ(Lookup(ids, true, 0, 25), kNotFound);   // between two entries
    EXPECT_EQ(Lookup(ids, true, 0, 99), kNotFound);   // after the last entry
}

TEST(JMessageResource, SortedTableOfOneEntry) {
    const std::vector<u32> ids = {1234};
    EXPECT_EQ(Lookup(ids, true, 0, 1234), 0);
    EXPECT_EQ(Lookup(ids, true, 0, 1235), kNotFound);
}

// --- Unsorted tables (linear search) ---------------------------------------------------------

TEST(JMessageResource, UnsortedTableFindsEveryEntry) {
    const std::vector<u32> ids = {40, 10, 30, 20, 50};
    for (size_t i = 0; i < ids.size(); i++) {
        EXPECT_EQ(Lookup(ids, false, 0, ids[i]), i) << "id " << ids[i];
    }
}

TEST(JMessageResource, UnsortedTableReportsMissingIds) {
    const std::vector<u32> ids = {40, 10, 30};
    EXPECT_EQ(Lookup(ids, false, 0, 20), kNotFound);
}

// Entries that are adjacent 32-bit words must stay separate values. Read as one 64-bit value,
// the pair {0x00000001, 0x00000002} would be 0x0000000200000001 on a little-endian host.
TEST(JMessageResource, AdjacentEntriesAreNotMerged) {
    const std::vector<u32> ids = {1, 2, 3, 4, 5, 6};
    EXPECT_EQ(Lookup(ids, false, 0, 2), 1);
    EXPECT_EQ(Lookup(ids, false, 0, 4), 3);
    EXPECT_EQ(Lookup(ids, false, 0, 6), 5);
    EXPECT_EQ(Lookup(ids, true, 0, 3), 2);
}

// --- Form supplements: how (id, upper half) are packed into one table value ------------------

// Supplement 0: the table holds the plain 32-bit ID; the upper half must be zero.
TEST(JMessageResource, Supplement0UsesIdDirectly) {
    const std::vector<u32> ids = {100, 200, 300};
    bool valid = false;
    EXPECT_EQ(Lookup(ids, true, 0, 200, 0, &valid), 1);
    EXPECT_TRUE(valid);
}

// Supplement 1: 24-bit ID in the high bits, 8-bit upper half in the low bits.
TEST(JMessageResource, Supplement1Packs24BitIdAnd8BitUpperHalf) {
    const u32 packed = (0x123456u << 8) | 0x7F;  // 0x1234567F
    const std::vector<u32> ids = {0x00000001, packed, 0xFFFFFF00};
    bool valid = false;
    EXPECT_EQ(Lookup(ids, true, 1, 0x123456, 0x7F, &valid), 1);
    EXPECT_TRUE(valid);
}

// Supplement 2: 16-bit ID in the high half, 16-bit upper half in the low half.
TEST(JMessageResource, Supplement2Packs16BitIdAnd16BitUpperHalf) {
    const u32 packed = (0xABCDu << 16) | 0x0042;  // 0xABCD0042
    const std::vector<u32> ids = {0x00010000, packed, 0xFFFF0000};
    bool valid = false;
    EXPECT_EQ(Lookup(ids, true, 2, 0xABCD, 0x0042, &valid), 1);
    EXPECT_TRUE(valid);
}

// Supplement 3: 8-bit ID in the top byte, 24-bit upper half below it.
TEST(JMessageResource, Supplement3Packs8BitIdAnd24BitUpperHalf) {
    const u32 packed = (0x5Au << 24) | 0x00BEEF;  // 0x5A00BEEF
    const std::vector<u32> ids = {0x01000000, packed, 0xFE000000};
    bool valid = false;
    EXPECT_EQ(Lookup(ids, true, 3, 0x5A, 0x00BEEF, &valid), 1);
    EXPECT_TRUE(valid);
}

// Supplement 4: only the upper half is used; the ID must be zero.
TEST(JMessageResource, Supplement4UsesUpperHalfOnly) {
    const std::vector<u32> ids = {5, 6, 7, 8};
    bool valid = false;
    EXPECT_EQ(Lookup(ids, true, 4, 0, 7, &valid), 2);
    EXPECT_TRUE(valid);
}

// --- Validity flag and degenerate inputs -----------------------------------------------------

TEST(JMessageResource, ReportsInvalidWhenAComponentDoesNotFit) {
    const std::vector<u32> ids = {1, 2, 3};
    bool valid = true;
    Lookup(ids, true, 0, 2, /*upper=*/1, &valid);  // supplement 0 requires upper == 0
    EXPECT_FALSE(valid);

    valid = true;
    Lookup(ids, true, 1, /*id=*/0x1000000, 0, &valid);  // does not fit in 24 bits
    EXPECT_FALSE(valid);

    valid = true;
    Lookup(ids, true, 4, /*id=*/1, 2, &valid);  // supplement 4 requires id == 0
    EXPECT_FALSE(valid);
}

TEST(JMessageResource, UnknownFormSupplementIsNotFound) {
    const std::vector<u32> ids = {1, 2, 3};
    EXPECT_EQ(Lookup(ids, true, /*supplement=*/9, 2), kNotFound);
}

// 0xFFFFFFFF is reserved to mean "no value", so it can never be found.
TEST(JMessageResource, ReservedValueIsNeverFound) {
    const std::vector<u32> ids = {1, 0xFFFFFFFE, 0xFFFFFFFF};
    EXPECT_EQ(Lookup(ids, false, 0, 0xFFFFFFFF), kNotFound);
}

TEST(JMessageResource, ResourceWithoutAMessageIdBlockFindsNothing) {
    JMessage::TResource resource;  // setData_block_messageID() never called
    EXPECT_EQ(resource.toMessageIndex_messageID(1, 0, nullptr), kNotFound);
}

// --- Randomised comparison with the oracle ---------------------------------------------------

// Many random tables, sorted and unsorted, queried with present and absent IDs. The seed is
// fixed so a failure is reproducible; the failure message names the table and the query.
TEST(JMessageResource, RandomTablesAgreeWithLinearSearch) {
    std::mt19937 rng(0x4D494431);  // "MID1"
    for (int round = 0; round < 500; round++) {
        const size_t count = 1 + rng() % 40;
        const bool sorted = rng() & 1;

        std::vector<u32> ids;
        while (ids.size() < count) {
            u32 value = rng() % 100000;
            if (std::find(ids.begin(), ids.end(), value) == ids.end()) {
                ids.push_back(value);
            }
        }
        if (sorted) {
            std::sort(ids.begin(), ids.end());
        }

        for (int q = 0; q < 20; q++) {
            const bool present = rng() % 3 != 0;
            const u32 query = present ? ids[rng() % count] : rng() % 100000;
            ASSERT_EQ(Lookup(ids, sorted, 0, query), ExpectedIndex(ids, query))
                << "round " << round << ", " << count << " ids, sorted=" << sorted
                << ", query " << query;
        }
    }
}
