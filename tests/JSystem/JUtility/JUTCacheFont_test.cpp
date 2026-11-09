// Tests for JUtility/JUTCacheFont.cpp: the page list that JUTCacheFont keeps for its glyph cache.
//
// What is under test
// ------------------
// A cache font owns a buffer of equally sized pages. Each page starts with a TGlyphCacheInfo,
// which holds `mPrev` and `mNext` pointers, so the pages form a doubly linked list.
// invalidiateAllCache() (sic) resets the whole cache: it links every page to its neighbours and
// records the first page (`field_0xa4`) and the last page (`field_0xa8`) in the font object.
//
// Why these tests exist
// ---------------------
// The links are pointers: 4 bytes on the GameCube, 8 bytes on a 64-bit host. The original code
// wrote them through an `int*`, which on a 64-bit host writes the two halves of `mPrev` and
// never touches `mNext`, leaving every page with a wrong link (5 of 5 pages in a reproduction).
// It compiled without complaint. The current code writes whole pointers; the first group of tests
// pins that down. The last test guards a related case: `field_0xa8` (the last page) used to be a
// `u32`, which truncates a pointer on 64-bit hosts; it is pointer-sized now and the test keeps it
// that way.
//
// How the object is made: the real constructor loads font resources, which is far more than this
// function needs. invalidiateAllCache() only reads and writes the five data members set below, so
// the test calls it on zeroed storage that is never constructed. That is technically undefined
// behaviour, which is why it is confined to this file and documented here; the members are
// private, hence the `#define private public` around the project headers.
//
// How it is built: the test links the real JUtility code (see tests/JSystem/CMakeLists.txt).

#include <gtest/gtest.h>

// Standard headers first, so the access hack below only applies to project headers.
#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <numbers>
#include <vector>

#define private public
#include "JSystem/JUtility/JUTCacheFont.h"
#undef private

#include "support/load_time_stubs.hpp"
#include "support/os_stubs.hpp"

namespace {

using Page = JUTCacheFont::TGlyphCacheInfo;

// A font whose page buffer holds `pageCount` pages `stride` bytes apart, after the reset.
class ResetCache {
public:
    ResetCache(u32 pageCount, int stride)
        : pageCount_(pageCount),
          stride_(stride),
          // aligned_alloc requires the size to be a multiple of the alignment.
          bytes_((pageCount * static_cast<size_t>(stride) + 4095) & ~size_t{4095}),
          buffer_(static_cast<u8*>(std::aligned_alloc(4096, bytes_))) {
        // Poison the buffer so a link that is never written is distinguishable from a null link.
        std::memset(buffer_, 0xAB, bytes_);
        std::memset(storage_, 0, sizeof(storage_));

        font_ = reinterpret_cast<JUTCacheFont*>(storage_);
        font_->mCacheBuffer = buffer_;
        font_->field_0x94 = stride;
        font_->mCachePage = pageCount;
        font_->invalidiateAllCache();
    }
    ~ResetCache() { std::free(buffer_); }
    ResetCache(const ResetCache&) = delete;
    ResetCache& operator=(const ResetCache&) = delete;

    Page* page(u32 index) const { return reinterpret_cast<Page*>(buffer_ + index * static_cast<size_t>(stride_)); }
    JUTCacheFont& font() const { return *font_; }
    u32 pageCount() const { return pageCount_; }

private:
    u32 pageCount_;
    int stride_;
    size_t bytes_;
    u8* buffer_;
    alignas(JUTCacheFont) unsigned char storage_[sizeof(JUTCacheFont)];
    JUTCacheFont* font_;
};

}  // namespace

// The two links must sit side by side, one pointer wide each, whatever the pointer width: the
// original wrote them as two ints and relied on that being the same thing.
TEST(JUTCacheFont, PageLinksAreWholePointers) {
    EXPECT_EQ(offsetof(Page, mNext), sizeof(Page*));
    EXPECT_EQ(sizeof(Page::mPrev), sizeof(void*));
}

// Every page points back to its predecessor and forward to its successor; the ends are null.
TEST(JUTCacheFont, ResetLinksPagesIntoADoublyLinkedList) {
    ResetCache cache(5, 0x1000);
    for (u32 i = 0; i < cache.pageCount(); i++) {
        Page* expectedPrev = i == 0 ? nullptr : cache.page(i - 1);
        Page* expectedNext = i + 1 == cache.pageCount() ? nullptr : cache.page(i + 1);
        EXPECT_EQ(cache.page(i)->mPrev, expectedPrev) << "page " << i << " mPrev";
        EXPECT_EQ(cache.page(i)->mNext, expectedNext) << "page " << i << " mNext";
    }
}

// Following mNext from the first page visits every page exactly once, in order.
TEST(JUTCacheFont, WalkingForwardVisitsEveryPageOnce) {
    ResetCache cache(7, 0x800);
    u32 visited = 0;
    for (Page* p = cache.page(0); p != nullptr && visited <= cache.pageCount(); p = p->mNext) {
        ASSERT_EQ(p, cache.page(visited)) << "step " << visited;
        visited++;
    }
    EXPECT_EQ(visited, cache.pageCount());
}

TEST(JUTCacheFont, ASinglePageHasNoNeighbours) {
    ResetCache cache(1, 0x1000);
    EXPECT_EQ(cache.page(0)->mPrev, nullptr);
    EXPECT_EQ(cache.page(0)->mNext, nullptr);
}

// The font remembers where the list starts and has no pages in use after a reset.
TEST(JUTCacheFont, ResetRecordsTheFirstPageAndClearsTheUsedList) {
    ResetCache cache(4, 0x1000);
    EXPECT_EQ(cache.font().field_0xa4, cache.page(0));
    EXPECT_EQ(cache.font().field_0x9c, nullptr);
    EXPECT_EQ(cache.font().field_0xa0, nullptr);
}

// field_0xa8 must hold a whole address. When it was a u32 this only passed if the buffer happened
// to lie below 4 GiB, so the width itself is asserted as well as the value.
TEST(JUTCacheFont, ResetRecordsTheLastPageWithoutTruncatingItsAddress) {
    static_assert(sizeof(JUTCacheFont::field_0xa8) >= sizeof(void*),
                  "field_0xa8 stores a page address and must be pointer-sized");
    ResetCache cache(4, 0x1000);
    EXPECT_EQ(static_cast<uintptr_t>(cache.font().field_0xa8),
              reinterpret_cast<uintptr_t>(cache.page(3)));
}
