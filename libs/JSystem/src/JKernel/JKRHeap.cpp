/**
 * JKRHeap.cpp
 * JSystem Heap Framework
 */

#include "JSystem/JSystem.h" // IWYU pragma: keep

#include "JSystem/JKernel/JKRHeap.h"

#include <cassert>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JUtility/JUTException.h"

#if DEBUG
u8 JKRValue_DEBUGFILL_NOTUSE = 0xFD;
u8 JKRValue_DEBUGFILL_NEW = 0xCD;
u8 JKRValue_DEBUGFILL_DELETE = 0xDD;
#endif

bool JKRHeap::sDefaultFillFlag = true;

JKRHeap* JKRHeap::sSystemHeap;

JKRHeap* JKRHeap::sCurrentHeap;

JKRHeap* JKRHeap::sRootHeap;

#if PLATFORM_WII || PLATFORM_SHIELD
JKRHeap* JKRHeap::sRootHeap2;
#endif

JKRErrorHandler JKRHeap::mErrorHandler;

static bool data_80451380;

JKRHeap::JKRHeap(void* data, const u32 size, JKRHeap* parent, const bool errorFlag)
    : JKRDisposer(), mChildTree(this), mDisposerList() {
    OSInitMutex(&mMutex);
    mSize = size;
    mStart = static_cast<u8*>(data);
    mEnd = static_cast<u8*>(data) + size;

    if (parent == nullptr) {
        // Record which thread is creating this heap
        mHomeThread = std::this_thread::get_id();
        becomeSystemHeap();
        becomeCurrentHeap();
    } else {
        parent->mChildTree.appendChild(&mChildTree);

        if (sSystemHeap == sRootHeap) {
            becomeSystemHeap();
        }

        if (sCurrentHeap == sRootHeap) {
            becomeCurrentHeap();
        }
    }

    mErrorFlag = errorFlag;
    if (mErrorFlag == true && mErrorHandler == nullptr) {
        mErrorHandler = JKRDefaultMemoryErrorRoutine;
    }

    mDebugFill = sDefaultFillFlag;
    mCheckMemoryFilled = data_80451380;
    mInitFlag = false;
}

JKRHeap::~JKRHeap() {
    mChildTree.getParent()->removeChild(&mChildTree);
    const JSUTree<JKRHeap>* nextRootHeap = sRootHeap->mChildTree.getFirstChild();

    if (sCurrentHeap == this) {
        sCurrentHeap = !nextRootHeap ? sRootHeap : nextRootHeap->getObject();
    }

    if (sSystemHeap == this) {
        sSystemHeap = !nextRootHeap ? sRootHeap : nextRootHeap->getObject();
    }
}

void* JKRHeap::mCodeStart;

void* JKRHeap::mCodeEnd;

void* JKRHeap::mUserRamStart;

void* JKRHeap::mUserRamEnd;

u32 JKRHeap::mMemorySize;

JKRHeap::JKRAllocCallback JKRHeap::sAllocCallback;

JKRHeap::JKRFreeCallback JKRHeap::sFreeCallback;

JKRHeap* JKRHeap::getParent() const {
    if (const auto parentTree = mChildTree.getParent(); parentTree != nullptr) {
        return parentTree->getObject();
    }
    return nullptr;
}

bool JKRHeap::initArena(char** memory, u32* size, int maxHeaps) {
    void* arenaLo = OSGetArenaLo();
    void* arenaHi = OSGetArenaHi();
#if !PLATFORM_GCN
    OSReport("original arenaLo = %p arenaHi = %p\n", arenaLo, arenaHi);
#endif
    if (arenaLo == arenaHi)
        return false;

    arenaLo = OSInitAlloc(arenaLo, arenaHi, maxHeaps);
    arenaLo = reinterpret_cast<void*>(ALIGN_NEXT(reinterpret_cast<uintptr_t>(arenaLo), 0x20));
    arenaHi = reinterpret_cast<void*>(ALIGN_PREV(reinterpret_cast<uintptr_t>(arenaHi), 0x20));

    auto* codeStart = static_cast<OSBootInfo*>(OSPhysicalToCached(0));
    mCodeStart = codeStart;
    mCodeEnd = arenaLo;

    mUserRamStart = arenaLo;
    mUserRamEnd = arenaHi;
    mMemorySize = codeStart->memorySize;

    OSSetArenaLo(arenaHi);
    OSSetArenaHi(arenaHi);

    *memory = static_cast<char*>(arenaLo);
    *size = reinterpret_cast<uintptr_t>(arenaHi) - reinterpret_cast<uintptr_t>(arenaLo);
    return true;
}

#if PLATFORM_WII || PLATFORM_SHIELD
bool JKRHeap::initArena2(char** memory, u32* size, int maxHeaps) {
    void* arenaLo = OSGetMEM2ArenaLo();
    void* arenaHi = OSGetMEM2ArenaHi();
#if !PLATFORM_GCN
    OSReport("original arenaLo = %p arenaHi = %p\n", arenaLo, arenaHi);
#endif
    if (arenaLo == arenaHi) {
        return false;
    }
    arenaLo = (void*)0x91100000;
    arenaHi = (void*)ALIGN_PREV(uintptr_t(arenaHi), 32);
    OSSetMEM2ArenaLo(arenaHi);
    OSSetMEM2ArenaHi(arenaHi);
    *memory = (char*)arenaLo;
    *size = uintptr_t(arenaHi) - uintptr_t(arenaLo);
    return true;
}
#endif

JKRHeap* JKRHeap::becomeSystemHeap() {
    JKRHeap* prev = sSystemHeap;
    sSystemHeap = this;
    return prev;
}

JKRHeap* JKRHeap::becomeCurrentHeap() {
    assert(this->current_thread_is_home());
    JKRHeap* prev = sCurrentHeap;
    sCurrentHeap = this;
    return prev;
}

void JKRHeap::destroy() {
    do_destroy();
}

static void dummy1(JKRHeap* heap) {
    JUT_ASSERT(0, heap != 0);
}

void* JKRHeap::alloc(const u32 size, const int alignment, JKRHeap* heap) {
    if (heap != nullptr) {
        return heap->alloc(size, alignment);
    }
    if (const auto currentHeap = getCurrentHeap(); currentHeap != nullptr) {
        return currentHeap->alloc(size, alignment);
    }

    return nullptr;
}

void* JKRHeap::alloc(u32 size, int alignment) {
    if (mInitFlag) {
        JUT_WARN(393, "alloc %x byte in heap %x", size, this);
    }
    void* mem = do_alloc(size, alignment);
#if DEBUG
    if (sAllocCallback) {
        sAllocCallback(size, alignment, this, mem);
    }
#endif
    return mem;
}

/**
 * Memory resource API version of `alloc`
 *
 * @param bytes count of bytes to allocate
 * @param alignment
 * @return pointer to the start of the allocated memory on success.
 */
void* JKRHeap::do_allocate(const std::size_t bytes, std::size_t alignment) {
    // Set alignment to the default if it was set to 0
    alignment = alignment == 0 ? alignof(std::max_align_t) : alignment;
    if (alignment >= 128 || (alignment & (alignment - 1)) != 0) {
        // alignment is too large for this engine or not a power of 2
        std::abort();
    }

    if (bytes > std::numeric_limits<uint32_t>::max()) {
        // Requested size too large for this engine
        std::abort();
    }

    void* mem = this->alloc(static_cast<uint32_t>(bytes), static_cast<int>(alignment));
    if (mem == std::nullptr_t()) {
        // This method is not allowed to return 'null'
        std::abort();
    }
    return mem;
}

void JKRHeap::free(void* ptr, JKRHeap* heap) {
    if (!heap) {
        heap = findFromRoot(ptr);
        if (!heap)
            return;
    }

    heap->free(ptr);
}

void JKRHeap::free(void* ptr) {
    if (mInitFlag) {
        JUT_WARN(441, "free %x in heap %x", ptr, this);
    }
#if DEBUG
    if (sFreeCallback) {
        sFreeCallback(ptr, this);
    }
#endif
    do_free(ptr);
}

/**
 * Memory resource API version of `free`
 *
 * @param ptr pointer to bytes previously allocated from this heap
 */
void JKRHeap::do_deallocate(void* ptr, size_t, size_t) noexcept {
    this->free(ptr);
}

bool JKRHeap::do_is_equal(const memory_resource& other) const noexcept {
    return this == &other;
}

void JKRHeap::callAllDisposer() {
    JSUListIterator<JKRDisposer> iterator;

    while ((iterator = mDisposerList.getFirst()) != mDisposerList.getEnd()) {
        iterator->~JKRDisposer();
    }
}

void JKRHeap::freeAll() {
    if (mInitFlag) {
        JUT_WARN(493, "freeAll in heap %x", this);
    }
    do_freeAll();
}

void JKRHeap::freeTail() {
    if (mInitFlag) {
        JUT_WARN(507, "freeTail in heap %x", this);
    }
    do_freeTail();
}

static void dummy2() {
    OS_REPORT("fillFreeArea in heap %x");
}

s32 JKRHeap::resize(void* ptr, u32 size, JKRHeap* heap) {
    if (!heap) {
        heap = findFromRoot(ptr);
        if (!heap)
            return -1;
    }

    return heap->resize(ptr, size);
}

s32 JKRHeap::resize(void* ptr, u32 size) {
    if (mInitFlag) {
        JUT_WARN(567, "resize block %x into %x in heap %x", ptr, size, this);
    }
    return do_resize(ptr, size);
}

s32 JKRHeap::getSize(void* ptr, JKRHeap* heap) {
    if (!heap) {
        heap = findFromRoot(ptr);
        if (!heap)
            return -1;
    }

    return heap->getSize(ptr);
}

s32 JKRHeap::getSize(void* ptr) {
    return do_getSize(ptr);
}

s32 JKRHeap::getFreeSize() {
    return do_getFreeSize();
}

void* JKRHeap::getMaxFreeBlock() {
    return do_getMaxFreeBlock();
}

s32 JKRHeap::getTotalFreeSize() {
    return do_getTotalFreeSize();
}

s32 JKRHeap::changeGroupID(u8 groupID) {
    if (mInitFlag) {
        JUT_WARN(646, "change heap ID into %x in heap %x", groupID, this);
    }
    return do_changeGroupID(groupID);
}

u8 JKRHeap::getCurrentGroupId() {
    return do_getCurrentGroupId();
}

u32 JKRHeap::getMaxAllocatableSize(int alignment) {
    const u32 maxFreeBlock = static_cast<u32>(
        reinterpret_cast<uintptr_t>(getMaxFreeBlock()));  // only the low 4 bits are used
    const u32 ptrOffset = (alignment - 1) & alignment - (maxFreeBlock & 0xf);
    return ~(alignment - 1) & (getFreeSize() - ptrOffset);
}

JKRHeap* JKRHeap::findFromRoot(void* ptr) {
    if (sRootHeap == nullptr) {
        return nullptr;
    }

    if (sRootHeap->mStart <= ptr && ptr < sRootHeap->mEnd) {
        return sRootHeap->find(ptr);
    }
#if PLATFORM_WII || PLATFORM_SHIELD
    if (sRootHeap2->mStart <= ptr && ptr < sRootHeap2->mEnd) {
        return sRootHeap2->find(ptr);
    }
#endif
    return sRootHeap->findAllHeap(ptr);
}

JKRHeap* JKRHeap::find(void* memory) const {
    if (mStart <= memory && memory < mEnd) {
        if (mChildTree.getNumChildren() != 0) {
            for (JSUTreeIterator iterator(mChildTree.getFirstChild());
                 iterator != mChildTree.getEndChild(); ++iterator)
            {
                if (JKRHeap* result = iterator->find(memory)) {
                    return result;
                }
            }
        }

        return const_cast<JKRHeap*>(this);
    }

    return nullptr;
}

JKRHeap* JKRHeap::findAllHeap(void* ptr) const {
    if (mChildTree.getNumChildren() != 0) {
        for (JSUTreeIterator iterator(mChildTree.getFirstChild());
             iterator != mChildTree.getEndChild(); ++iterator)
        {
            if (JKRHeap* result = iterator->findAllHeap(ptr)) {
                return result;
            }
        }
    }

    if (mStart <= ptr && ptr < mEnd) {
        return const_cast<JKRHeap*>(this);
    }

    return nullptr;
}

void JKRHeap::dispose_subroutine(const uintptr_t start, const uintptr_t end) const {
    JSUListIterator<JKRDisposer> next_iterator(static_cast<JSULink<JKRDisposer>*>(nullptr));
    JSUListIterator<JKRDisposer> it = mDisposerList.getFirst();
    while (it != mDisposerList.getEnd()) {
        if (const JKRDisposer* disposer = it.getObject();
            reinterpret_cast<void*>(start) <= disposer && disposer < reinterpret_cast<void*>(end)) {
            it->~JKRDisposer();

            if (next_iterator == JSUListIterator<JKRDisposer>(static_cast<JSULink<JKRDisposer>*>(nullptr))) {
                it = mDisposerList.getFirst();
                continue;
            }
            it = next_iterator;
            ++it;
            continue;
        }
        next_iterator = it;
        ++it;
    }
}

bool JKRHeap::dispose(void* ptr, const u32 size) const {
    dispose_subroutine(reinterpret_cast<uintptr_t>(ptr), reinterpret_cast<uintptr_t>(ptr) + size);
    return false;
}

void JKRHeap::dispose(void* begin, void* end) const {
    dispose_subroutine(reinterpret_cast<uintptr_t>(begin), reinterpret_cast<uintptr_t>(end));
}

void JKRHeap::dispose() const {
    JSUListIterator<JKRDisposer> iterator;
    while ((iterator = mDisposerList.getFirst()) != mDisposerList.getEnd()) {
        iterator->~JKRDisposer();
    }
}

void JKRHeap::copyMemory(void* dst, void* src, const u32 size) {
    u32 count = (size + 3) / 4;

    auto dst_32 = static_cast<u32*>(dst);
    auto src_32 = static_cast<u32*>(src);
    while (count-- > 0) {
        *dst_32++ = *src_32++;
    }
}

void JKRDefaultMemoryErrorRoutine(void* heap, u32 size, int alignment) {
    OS_REPORT("Error: Cannot allocate memory %d(0x%x)byte in %d byte alignment from %08x\n", size,
             size, alignment, heap);
#if PLATFORM_GCN
    JUTException::panic(__FILE__, 831, "abort\n");
#else
    JUTException::panic(__FILE__, 912, "abort\n");
#endif
}

bool JKRHeap::setErrorFlag(const bool errorFlag) {
    const bool prev = mErrorFlag;
    mErrorFlag = errorFlag;
    return prev;
}

JKRErrorHandler JKRHeap::setErrorHandler(const JKRErrorHandler errorHandler) {
    const JKRErrorHandler prev = mErrorHandler;

    mErrorHandler = !errorHandler ? JKRDefaultMemoryErrorRoutine : errorHandler;

    return prev;
}

void JKRHeap::fillMemory(u8* dst, const u32 size, const u8 val) {
    const auto ptr = reinterpret_cast<uintptr_t>(dst);
    memset(dst, val, size);
    DCFlushRange(reinterpret_cast<void*>(ALIGN_PREV(ptr, 32)), ALIGN_NEXT(size, 32));
}

bool JKRHeap::checkMemoryFilled(u8* mem, const u32 size, const u8 val) {
    void* ptr = mem;
    bool result = true;
    for (int i = 0; i < size; i++) {
        if (val == mem[i]) {
            continue;
        }
        result = false;
        if (fillcheck_dispcount <= 0) {
            continue;
        }
        fillcheck_dispcount--;
        JUT_WARN(999, "**** checkMemoryFilled:\n address %08x size %x:\n (%08x = %02x)\n", mem, size, mem + i, mem[i]);
        if (data_8074A8D0_debug) {
            break;
        }
    }
    return result;
}

const JKRHeap* JKRHeap::getRoot(const JKRHeap* heap) noexcept{
    const auto parent = heap->getParent();
    if (!parent) {
        return heap;
    }
    return getRoot(parent);
}

bool JKRHeap::current_thread_is_home() const noexcept{
    const auto root = getRoot(this);
    assert(root->mHomeThread.has_value()); // The 'root' heap not having a thread ID is a programming error
    const auto rootThreadId = root->mHomeThread.value();
    return std::this_thread::get_id() == rootThreadId;
}

JKRHeap* JKRHeap::getCurrentHeap() {
    const auto currentHeap = sCurrentHeap;
    assert(!currentHeap || currentHeap->current_thread_is_home());
    return currentHeap;
}

void JKRHeap::setCurrentHeap(JKRHeap* heap) {
    assert(!heap || heap->current_thread_is_home());
    sCurrentHeap = heap;
}

bool JKRHeap::isSubHeap(JKRHeap* heap) const {
    if (!heap)
        return false;

    if (mChildTree.getNumChildren() != 0) {
        for (JSUTreeIterator<JKRHeap> iterator = mChildTree.getFirstChild(); iterator != mChildTree.getEndChild();
             ++iterator)
        {
            if (iterator.getObject() == heap) {
                return true;
            }

            if (iterator.getObject()->isSubHeap(heap)) {
                return true;
            }
        }
    }

    return false;
}

#ifndef __MWERKS__
// On the console a JKR heap always exists before the first allocation, so these operators can hand
// everything to it. On a host there is none during static initialisation (and in unit tests that
// link the real JKernel code), where JKRHeap::alloc() would return null and the process would
// crash on its first `new`, including inside the standard library. So when no JKR heap is current
// the standard allocator is used instead, and operator delete releases any pointer that no JKR heap
// owns with std::free. Neither changes what happens once a heap is in use.
static void* HostAllocate(size_t size, int alignment) {
    size_t align = alignment < 0 ? static_cast<size_t>(-alignment) : static_cast<size_t>(alignment);
    if (align < alignof(std::max_align_t)) {
        align = alignof(std::max_align_t);
    }
    while ((align & (align - 1)) != 0) {  // round up to a power of two
        align += align & ~(align - 1);
    }
    return std::aligned_alloc(align, ((size == 0 ? 1 : size) + align - 1) / align * align);
}

#endif

void* operator new(const size_t size) {
#ifndef __MWERKS__
    if (JKRHeap::getCurrentHeap() == nullptr) {
        return HostAllocate(size, 4);
    }
#endif
    return JKRHeap::alloc(size, 4, nullptr);
}

void* operator new(const size_t size, const int alignment) {
#ifndef __MWERKS__
    if (JKRHeap::getCurrentHeap() == nullptr) {
        return HostAllocate(size, alignment);
    }
#endif
    return JKRHeap::alloc(size, alignment, nullptr);
}

void* operator new(const size_t size, JKRHeap* heap, const int alignment) {
    return JKRHeap::alloc(size, alignment, heap);
}

void* operator new[](const size_t size) {
#ifndef __MWERKS__
    if (JKRHeap::getCurrentHeap() == nullptr) {
        return HostAllocate(size, 4);
    }
#endif
    return JKRHeap::alloc(size, 4, nullptr);
}

void* operator new[](size_t size, int alignment) {
#ifndef __MWERKS__
    if (JKRHeap::getCurrentHeap() == nullptr) {
        return HostAllocate(size, alignment);
    }
#endif
    return JKRHeap::alloc(size, alignment, nullptr);
}

void* operator new[](const size_t size, JKRHeap* heap, const int alignment) {
    return JKRHeap::alloc(size, alignment, heap);
}

void operator delete(void* ptr) {
#ifndef __MWERKS__
    if (ptr != nullptr && JKRHeap::findFromRoot(ptr) == nullptr) {
        std::free(ptr);
        return;
    }
#endif
    JKRHeap::free(ptr, nullptr);
}

void operator delete[](void* ptr) {
#ifndef __MWERKS__
    if (ptr != nullptr && JKRHeap::findFromRoot(ptr) == nullptr) {
        std::free(ptr);
        return;
    }
#endif
    JKRHeap::free(ptr, nullptr);
}

s32 fillcheck_dispcount = 100;
bool data_8074A8D0_debug = true;

void JKRHeap::state_register(JKRHeap::TState* p, u32 id) const {
    JUT_ASSERT(1213, p != NULL);
    JUT_ASSERT(1214, p->getHeap() == this);
}

bool JKRHeap::state_compare(const JKRHeap::TState& r1, const JKRHeap::TState& r2) const {
    JUT_ASSERT(1222, r1.getHeap() == r2.getHeap());
    return r1.getCheckCode() == r2.getCheckCode();
}

void JKRHeap::state_dump(const JKRHeap::TState& p) const {
    JUT_LOG(1246, "check-code : 0x%08x", p.getCheckCode());
    JUT_LOG(1247, "id         : 0x%08x", p.getId());
    JUT_LOG(1248, "used size  : %u", p.getUsedSize());
}

static auto ARALT_AramStartAdr = reinterpret_cast<void*>(0x90000000);

void* JKRHeap::getAltAramStartAdr() { return ARALT_AramStartAdr; }

s32 JKRHeap::do_changeGroupID(u8 param_0) {
    return 0;
}

u8 JKRHeap::do_getCurrentGroupId() {
    return 0;
}
