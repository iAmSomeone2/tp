// Tests for JKernel/JKRArchivePub.cpp: JKRArchive::check_mount_already(), which decides whether an
// archive is already mounted so it can be shared instead of loaded again.
//
// What is under test
// ------------------
// Every mounted archive is registered in JKRFileLoader::sVolumeList. check_mount_already(key,
// heap) walks that list and returns the first archive of type 'RARC' whose key (`mEntryNum`) and
// heap both match, after incrementing its mount count; it returns null if there is none. A null
// `heap` argument means "the current heap".
//
// The key is what makes an archive unique. For an archive on the disc it is the DVD entry number
// (a small integer). For an archive mounted from memory (JKRMemArchive) it is the **address of the
// buffer**, so the key must be able to hold a pointer.
//
// Why these tests exist
// ---------------------
// The key used to be an s32. On a 64-bit host that truncates a buffer address, so two memory
// archives whose buffers differ only above bit 31 looked identical, and the second mount was
// silently handed the first archive's contents. The key (and the JKRArchive constructor and
// mEntryNum field that carry it) is now intptr_t. The tests build archives with keys that differ
// only in their upper 32 bits to pin that down; they are skipped on 32-bit hosts, where such
// addresses cannot exist.
//
// How it is built: the real JKernel code is linked (through tp::engine). Test archives are small
// JKRArchive subclasses that register themselves in the volume list without reading any data.
// Heaps are made-up addresses: check_mount_already only compares them, never dereferences them.

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "JSystem/JKernel/JKRArchive.h"
#include "JSystem/JKernel/JKRHeap.h"

#include "support/load_time_stubs.hpp"
#include "support/os_stubs.hpp"

namespace {

constexpr u32 kArchiveType = 'RARC';

// A JKRArchive with nothing behind it. Constructing it runs the real JKRArchive constructor
// (key, mount count of 1); registering it puts it in the global volume list, as mounting does.
class TestArchive : public JKRArchive {
public:
    TestArchive(intptr_t key, JKRHeap* heap, u32 volumeType = kArchiveType)
        : JKRArchive(key, MOUNT_MEM) {
        mHeap = heap;
        mVolumeType = volumeType;
        sVolumeList.append(&mFileLoaderLink);
    }

    u32 mountCount() const { return mMountCount; }

    void* fetchResource(SDIFileEntry*, u32*) override { return nullptr; }
    void* fetchResource(void*, u32, SDIFileEntry*, u32*) override { return nullptr; }
};

JKRHeap* FakeHeap(uintptr_t address) { return reinterpret_cast<JKRHeap*>(address); }

bool AddressFitsInAPointer(uintptr_t address) {
    return reinterpret_cast<uintptr_t>(reinterpret_cast<void*>(address)) == address;
}

// Owns a few archives for one test so the volume list is empty again afterwards.
class JKRArchivePubTest : public ::testing::Test {
protected:
    TestArchive& Add(intptr_t key, JKRHeap* heap, u32 volumeType = kArchiveType) {
        archives_.push_back(std::make_unique<TestArchive>(key, heap, volumeType));
        return *archives_.back();
    }

    JKRHeap* heapA = FakeHeap(0x1000);
    JKRHeap* heapB = FakeHeap(0x2000);

private:
    std::vector<std::unique_ptr<TestArchive>> archives_;
};

}  // namespace

TEST_F(JKRArchivePubTest, NothingMountedFindsNothing) {
    EXPECT_EQ(JKRArchive::check_mount_already(5, heapA), nullptr);
}

TEST_F(JKRArchivePubTest, FindsAnArchiveByKeyAndHeap) {
    TestArchive& archive = Add(5, heapA);
    EXPECT_EQ(JKRArchive::check_mount_already(5, heapA), &archive);
}

// A hit counts as another user of the archive.
TEST_F(JKRArchivePubTest, AHitIncrementsTheMountCount) {
    TestArchive& archive = Add(5, heapA);
    ASSERT_EQ(archive.mountCount(), 1u);
    JKRArchive::check_mount_already(5, heapA);
    EXPECT_EQ(archive.mountCount(), 2u);
    JKRArchive::check_mount_already(5, heapA);
    EXPECT_EQ(archive.mountCount(), 3u);
}

TEST_F(JKRArchivePubTest, AMissLeavesTheMountCountAlone) {
    TestArchive& archive = Add(5, heapA);
    EXPECT_EQ(JKRArchive::check_mount_already(6, heapA), nullptr);
    EXPECT_EQ(archive.mountCount(), 1u);
}

TEST_F(JKRArchivePubTest, TheKeyMustMatch) {
    Add(5, heapA);
    EXPECT_EQ(JKRArchive::check_mount_already(6, heapA), nullptr);
}

TEST_F(JKRArchivePubTest, TheHeapMustMatch) {
    Add(5, heapA);
    EXPECT_EQ(JKRArchive::check_mount_already(5, heapB), nullptr);
}

TEST_F(JKRArchivePubTest, PicksTheArchiveThatMatchesAmongSeveral) {
    Add(1, heapA);
    TestArchive& wanted = Add(2, heapB);
    Add(3, heapA);
    EXPECT_EQ(JKRArchive::check_mount_already(2, heapB), &wanted);
}

TEST_F(JKRArchivePubTest, SameKeyInDifferentHeapsAreDifferentArchives) {
    TestArchive& a = Add(7, heapA);
    TestArchive& b = Add(7, heapB);
    EXPECT_EQ(JKRArchive::check_mount_already(7, heapA), &a);
    EXPECT_EQ(JKRArchive::check_mount_already(7, heapB), &b);
}

// Other kinds of volume in the list (a plain file loader, say) are skipped even if their numbers
// happen to match.
TEST_F(JKRArchivePubTest, VolumesThatAreNotArchivesAreIgnored) {
    Add(5, heapA, /*volumeType=*/'ARCX');
    EXPECT_EQ(JKRArchive::check_mount_already(5, heapA), nullptr);
}

// With no heap argument the lookup uses the current heap. The current heap is a made-up address
// here, and the engine's operator new allocates from it once it is set, so it is only set around
// the call itself (and restored before anything can allocate or report a failure).
TEST_F(JKRArchivePubTest, ANullHeapMeansTheCurrentHeap) {
    TestArchive& inCurrent = Add(9, heapB);
    Add(9, heapA);

    JKRHeap* saved = JKRHeap::getCurrentHeap();
    JKRHeap::setCurrentHeap(heapB);
    JKRArchive* found = JKRArchive::check_mount_already(9, nullptr);
    JKRHeap::setCurrentHeap(saved);

    EXPECT_EQ(found, &inCurrent);
}

// --- Keys that do not fit in 32 bits (memory archives are keyed by buffer address) -----------

TEST_F(JKRArchivePubTest, KeysThatDifferOnlyInTheUpperBitsAreDifferent) {
    constexpr uintptr_t kKeyA = 0x100001000;
    constexpr uintptr_t kKeyB = 0x200001000;  // same low 32 bits as kKeyA
    if (!AddressFitsInAPointer(kKeyA) || !AddressFitsInAPointer(kKeyB)) {
        GTEST_SKIP() << "addresses above 4 GiB do not exist on this host";
    }
    TestArchive& a = Add(static_cast<intptr_t>(kKeyA), heapA);
    TestArchive& b = Add(static_cast<intptr_t>(kKeyB), heapA);

    EXPECT_EQ(JKRArchive::check_mount_already(static_cast<intptr_t>(kKeyA), heapA), &a);
    EXPECT_EQ(JKRArchive::check_mount_already(static_cast<intptr_t>(kKeyB), heapA), &b);
    EXPECT_EQ(a.mountCount(), 2u);
    EXPECT_EQ(b.mountCount(), 2u);
}

TEST_F(JKRArchivePubTest, ALargeKeyIsNotConfusedWithItsLow32Bits) {
    constexpr uintptr_t kBuffer = 0x123400000;
    if (!AddressFitsInAPointer(kBuffer)) {
        GTEST_SKIP() << "addresses above 4 GiB do not exist on this host";
    }
    Add(static_cast<intptr_t>(kBuffer), heapA);
    EXPECT_EQ(JKRArchive::check_mount_already(static_cast<intptr_t>(kBuffer & 0xFFFFFFFF), heapA),
              nullptr);
}

// The key field is wide enough to hold an address, whatever the host.
TEST(JKRArchive, TheKeyIsPointerSized) {
    static_assert(sizeof(JKRArchive::mEntryNum) >= sizeof(void*),
                  "a memory archive's key is its buffer address");
    SUCCEED();
}
