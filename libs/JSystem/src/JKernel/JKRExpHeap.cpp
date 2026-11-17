//
// JKRExpHeap
//

#include "JSystem/JSystem.h" // IWYU pragma: keep

#include "JSystem/JKernel/JKRExpHeap.h"

#include <assert.h>

#include <cstdlib>
#include "JSystem/JGadget/binary.h"
#include "JSystem/JSupport/JSupport.h"
#include "JSystem/JUtility/JUTConsole.h"
#include "JSystem/JUtility/JUTException.h"

JKRExpHeap* JKRExpHeap::createRoot(int maxHeaps, bool errorFlag) {
    JKRExpHeap* heap = nullptr;
    if (!sRootHeap) {
#if !PLATFORM_GCN
        u8* mem2;
        u32 mem2Size;
        initArena2((char**)&mem2, &mem2Size, 1);
        u32 local_1c = ALIGN_NEXT(sizeof(JKRExpHeap), 0x10);
        u8* local_20 = mem2 + local_1c;
        u32 local_24 = mem2Size - local_1c;
#if DEBUG
        if (local_20) {
            u8* local_28 = local_20;
            u32 local_2c = local_24;
            if (isDefaultDebugFill()) {
                JKRFillMemory(local_28, local_2c, JKRValue_DEBUGFILL_NOTUSE);
            }
        }
#endif
        JKRExpHeap* heap2 = new (mem2) JKRExpHeap(local_20, local_24, nullptr, errorFlag);
        sRootHeap2 = heap2;
        heap2->field_0x6e = true;
#endif
        void* mem1;
        u32 mem1Size;
        initArena(reinterpret_cast<char**>(&mem1), &mem1Size, maxHeaps);
        constexpr u32 local_3c = ALIGN_NEXT(sizeof(JKRExpHeap), 0x10);
        u8* start1 = static_cast<u8*>(mem1) + local_3c;
        const u32 alignedSize = mem1Size - local_3c;
#if DEBUG
        if (start1) {
            u8* local_4c = start1;
            u32 local_48 = alignedSize;
            if (isDefaultDebugFill()) {
                JKRFillMemory(local_4c, local_48, JKRValue_DEBUGFILL_NOTUSE);
            }
        }
#endif
        heap = new (mem1) JKRExpHeap(start1, alignedSize, nullptr, errorFlag);
        sRootHeap = heap;
    }
    heap->field_0x6e = true;
    return heap;
}

JKRExpHeap* JKRExpHeap::create(u32 size, JKRHeap* parent, bool errorFlag) {
    if (!parent) {
        parent = sRootHeap;
    }
    constexpr u32 expHeapSize = ALIGN_NEXT(sizeof(JKRExpHeap), 0x10);

    if (size == 0xffffffff) {
        size = parent->getMaxAllocatableSize(0x10);
    }

    const u32 alignedSize = ALIGN_PREV(size, 0x10);

    if (constexpr u32 blockSize = sizeof(CMemBlock); alignedSize < expHeapSize + blockSize)
        return nullptr;

    auto memory = static_cast<u8*>(JKRAllocFromHeap(parent, alignedSize, 0x10));
    u8* dataPtr = (memory + expHeapSize);
    if (!memory) {
        return nullptr;
    }

    auto* newHeap =
        new (memory) JKRExpHeap(dataPtr, alignedSize - expHeapSize, parent, errorFlag);

    if (newHeap == nullptr) {
        JKRFree(memory);
        return nullptr;
    }
#if DEBUG
    if (newHeap) {
        u8* local_30 = dataPtr + sizeof(CMemBlock);
        u32 local_34 = newHeap->mHeadFreeList->size;
        if (isDefaultDebugFill()) {
            JKRFillMemory(local_30, local_34, JKRValue_DEBUGFILL_NOTUSE);
        }
    }
#endif
    newHeap->field_0x6e = false;
    return newHeap;
}

JKRExpHeap* JKRExpHeap::create(void* ptr, const u32 size, JKRHeap* parent, const bool errorFlag) {
    JKRHeap* parent2;
    if (parent == nullptr) {
        parent2 = getRootHeap()->find(ptr);
        if (!parent2)
            return nullptr;
    } else {
        parent2 = parent;
    }

    JKRExpHeap* newHeap = nullptr;
    constexpr u32 expHeapSize = ALIGN_NEXT(sizeof(JKRExpHeap), 0x10);
    if (size < expHeapSize)
        return nullptr;

    auto r28 = static_cast<u8*>(ptr);
    u8* dataPtr = r28 + expHeapSize;
    const u32 alignedSize = ALIGN_PREV(reinterpret_cast<uintptr_t>(ptr) + size - reinterpret_cast<uintptr_t>(dataPtr), 0x10);
    if (r28) {
        newHeap = new (r28) JKRExpHeap(dataPtr, alignedSize, parent2, errorFlag);
    }
#if DEBUG
    if (newHeap) {
        u8* local_30 = dataPtr + sizeof(CMemBlock);
        u32 local_34 = newHeap->mHeadFreeList->size;
        if (isDefaultDebugFill()) {
            JKRFillMemory(local_30, local_34, JKRValue_DEBUGFILL_NOTUSE);
        }
    }
#endif
    newHeap->field_0x6e = true;
    newHeap->field_0x70 = ptr;
    newHeap->field_0x74 = size;
    return newHeap;
}

void JKRExpHeap::do_destroy() {
    if (!field_0x6e) {
        if (JKRHeap* heap = getParent()) {
            this->~JKRExpHeap();
            JKRFreeToHeap(heap, this);
        }
    } else {
        auto r28 = static_cast<u8*>(field_0x70);
        u32 r27 = field_0x74;
        this->~JKRExpHeap();
#if DEBUG
        if (mDebugFill) {
            JKRFillMemory(r28, r27, JKRValue_DEBUGFILL_NOTUSE);
        }
#endif
    }
}

JKRExpHeap::JKRExpHeap(void* data, const u32 size, JKRHeap* parent, const bool errorFlag)
    : JKRHeap(data, size, parent, errorFlag) {
    // Assert -- in debug builds -- that 'data' and 'size' are multiples of MIN_ALLOC_ALIGN
    assert(reinterpret_cast<size_t>(data) % MIN_ALLOC_ALIGN == 0);
    assert(size % MIN_ALLOC_ALIGN == 0);

    mAllocMode = 0;
    mCurrentGroupId = 0xff;
    mHeadFreeList = static_cast<CMemBlock*>(data);
    mTailFreeList = mHeadFreeList;
    mHeadFreeList->initiate(nullptr, nullptr, size - sizeof(CMemBlock), 0, 0);
    mHeadUsedList = nullptr;
    mTailUsedList = nullptr;
}

JKRExpHeap::~JKRExpHeap() {
    dispose();
}

void* JKRExpHeap::do_alloc(u32 size, const int alignment) {
#if DEBUG
    if (alignment) {
        u32 u = abs(alignment);
        JUT_CONFIRM(356, u < 0x80);
        JUT_CONFIRM(357, JGadget::binary::isPower2( u ));
    }
#endif

    lock();
    if (size < MIN_ALLOC_ALIGN) {
        size = MIN_ALLOC_ALIGN;
    }

    void* ptr;
    if (alignment >= 0) {
        if (alignment <= MIN_ALLOC_ALIGN) {
            ptr = allocFromHead(size);
        } else {
            ptr = allocFromHead(size, alignment);
        }
    } else {
        if (-alignment <= MIN_ALLOC_ALIGN) {
            ptr = allocFromTail(size);
        } else {
            ptr = allocFromTail(size, -alignment);
        }
    }
#if DEBUG
    if (ptr) {
        CMemBlock* block = (CMemBlock*)((u8*)ptr - sizeof(CMemBlock));
        if (mCheckMemoryFilled) {
            JKRHeap::checkMemoryFilled((u8*)ptr, block->size, JKRValue_DEBUGFILL_DELETE);
        }
        if (mDebugFill) {
            JKRFillMemory((u8*)ptr, block->size, JKRValue_DEBUGFILL_NEW);
        }
        if (block->getAlignment() && mDebugFill) {
            JKRFillMemory((u8*)ptr - sizeof(CMemBlock) - block->getAlignment(), block->getAlignment(), JKRValue_DEBUGFILL_NEW);
        }
    }
#endif

    if (ptr == nullptr) {
        JUTWarningConsole_f(":::cannot alloc memory (0x%x byte).\n", size);
        if (getErrorFlag() == true) {
            callErrorHandler(this, size, alignment);
        }
    }
    unlock();

    return ptr;
}

static u32 DBfoundSize;

static u32 DBfoundOffset;

static JKRExpHeap::CMemBlock* DBfoundBlock;

static JKRExpHeap::CMemBlock* DBnewFreeBlock;

static JKRExpHeap::CMemBlock* DBnewUsedBlock;

void* JKRExpHeap::allocFromHead(u32 size, const int align) {
    size = ALIGN_NEXT(size, MIN_ALLOC_ALIGN);
    int foundSize = -1;
    u32 foundOffset = 0;
    CMemBlock* foundBlock = nullptr;

    for (CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        const u32 offset =
            ALIGN_PREV(align - 1 + reinterpret_cast<uintptr_t>(block->getContent()), align) - reinterpret_cast<uintptr_t>(block->getContent());
        if (block->size < size + offset) {
            continue;
        }

        if (foundSize <= block->size) {
            continue;
        }

        foundSize = block->size;
        foundBlock = block;
        foundOffset = offset;
        if (mAllocMode != 0) {
            break;
        }

        if (foundSize == size) {
            break;
        }
    }

    DBfoundSize = foundSize;
    DBfoundOffset = foundOffset;
    DBfoundBlock = foundBlock;

    if (foundBlock) {
        CMemBlock* newUsedBlock;
        CMemBlock* newFreeBlock;
        if (foundOffset >= sizeof(CMemBlock)) {
            CMemBlock* prev = foundBlock->mPrev;
            CMemBlock* next = foundBlock->mNext;
            newUsedBlock = foundBlock->allocFore(foundOffset - sizeof(CMemBlock), 0, 0, 0, 0);

            if (newUsedBlock) {
                newFreeBlock = newUsedBlock->allocFore(size, mCurrentGroupId, 0, 0, 0);
            } else {
                newFreeBlock = nullptr;
            }

            if (newFreeBlock) {
                setFreeBlock(foundBlock, prev, newFreeBlock);
            } else {
                setFreeBlock(foundBlock, prev, next);
            }

            if (newFreeBlock) {
                setFreeBlock(newFreeBlock, foundBlock, next);
            }

            appendUsedList(newUsedBlock);
            DBnewFreeBlock = newFreeBlock;
            DBnewUsedBlock = newUsedBlock;
            return newUsedBlock->getContent();
        } else {
            if (foundOffset != 0) {
                CMemBlock* prev = foundBlock->mPrev;
                CMemBlock* next = foundBlock->mNext;
                removeFreeBlock(foundBlock);
                newUsedBlock = reinterpret_cast<CMemBlock*>(
                    reinterpret_cast<uintptr_t>(foundBlock) + foundOffset);
                newUsedBlock->size = foundBlock->size - foundOffset;
                newFreeBlock =
                    newUsedBlock->allocFore(size, mCurrentGroupId, static_cast<u8>(foundOffset), 0, 0);
                if (newFreeBlock) {
                    setFreeBlock(newFreeBlock, prev, next);
                }
                appendUsedList(newUsedBlock);
                return newUsedBlock->getContent();
            } else {
                CMemBlock* prev = foundBlock->mPrev;
                CMemBlock* next = foundBlock->mNext;

                newFreeBlock = foundBlock->allocFore(size, mCurrentGroupId, 0, 0, 0);
                removeFreeBlock(foundBlock);
                if (newFreeBlock) {
                    setFreeBlock(newFreeBlock, prev, next);
                }
                appendUsedList(foundBlock);
                return foundBlock->getContent();
            }
        }
    }

    return nullptr;
}

void* JKRExpHeap::allocFromHead(u32 size) {
    size = ALIGN_NEXT(size, MIN_ALLOC_ALIGN);
    s32 foundSize = -1;
    CMemBlock* foundBlock = nullptr;
    for (CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        if (block->size < size) {
            continue;
        }

        if (foundSize <= block->size) {
            continue;
        }

        foundSize = block->size;
        foundBlock = block;
        if (mAllocMode != 0) {
            break;
        }

        if (foundSize == size) {
            break;
        }
    }

    if (foundBlock) {
        if (CMemBlock* newBlock = foundBlock->allocFore(size, mCurrentGroupId, 0, 0, 0)) {
            setFreeBlock(newBlock, foundBlock->mPrev, foundBlock->mNext);
        } else {
            removeFreeBlock(foundBlock);
        }
        appendUsedList(foundBlock);
        return foundBlock->getContent();
    }
    return nullptr;
}

void* JKRExpHeap::allocFromTail(u32 size, int align) {
    u32 offset = 0;
    CMemBlock* foundBlock = nullptr;
    CMemBlock* newBlock = nullptr;
    u32 usedSize = 0;

    for (CMemBlock* block = mTailFreeList; block; block = block->mPrev)         {
        const uintptr_t start = ALIGN_PREV(
            reinterpret_cast<uintptr_t>(block->getContent()) + block->size - size, align);
        usedSize = reinterpret_cast<uintptr_t>(block->getContent()) + block->size - start;
        if (block->size >= usedSize) {
            foundBlock = block;
            offset = block->size - usedSize;
            newBlock = reinterpret_cast<CMemBlock*>(start) - 1;
            break;
        }
    }

    if (foundBlock != nullptr) {
        if (offset >= sizeof(CMemBlock)) {
            newBlock->initiate(nullptr, nullptr, usedSize, mCurrentGroupId, -0x80);
            foundBlock->size = foundBlock->size - usedSize - sizeof(CMemBlock);
            appendUsedList(newBlock);
            return newBlock->getContent();
        } else {
            if (offset != 0) {
                removeFreeBlock(foundBlock);
                newBlock->initiate(nullptr, nullptr, usedSize, mCurrentGroupId, offset | 0x80);
                appendUsedList(newBlock);
                return newBlock->getContent();
            } else {
                removeFreeBlock(foundBlock);
                newBlock->initiate(nullptr, nullptr, usedSize, mCurrentGroupId, -0x80);
                appendUsedList(newBlock);
                return newBlock->getContent();
            }
        }
    }

    return nullptr;
}

void* JKRExpHeap::allocFromTail(u32 size) {
    size = ALIGN_NEXT(size, MIN_ALLOC_ALIGN);
    CMemBlock* foundBlock = nullptr;
    for (CMemBlock* block = mTailFreeList; block; block = block->mPrev) {
        if (block->size >= size) {
            foundBlock = block;
            break;
        }
    }

    if (foundBlock != nullptr) {
        CMemBlock* freeBlock;
        CMemBlock* usedBlock = foundBlock->allocBack(size, 0, 0, mCurrentGroupId, 0);
        if (usedBlock) {
            freeBlock = foundBlock;
        } else {
            removeFreeBlock(foundBlock);
            usedBlock = foundBlock;
            freeBlock = nullptr;
        }

        if (freeBlock) {
            setFreeBlock(freeBlock, foundBlock->mPrev, foundBlock->mNext);
        }
        appendUsedList(usedBlock);
        return usedBlock->getContent();
    }
    return nullptr;
}

void JKRExpHeap::do_free(void* ptr) {
    if (!ptr) return;
    lock();
    const auto inBlock = contains(ptr);
    auto* block = inBlock ? CMemBlock::getHeapBlock(ptr) : nullptr;
    if (block) {
        block->free(this);
    } else {
        JUT_WARN(921, "free: memblock %x not in heap %x", ptr, this);
    }
    unlock();
    assert(block != nullptr);
}

static void dummy() {
    OS_REPORT("newSize > 0");
}

void JKRExpHeap::do_freeAll() {
    lock();
    JKRHeap::callAllDisposer();
    mHeadFreeList = reinterpret_cast<CMemBlock*>(mStart);
    mTailFreeList = mHeadFreeList;
    mHeadFreeList->initiate(nullptr, nullptr, mSize - sizeof(CMemBlock), 0, 0);
    mHeadUsedList = nullptr;
    mTailUsedList = nullptr;
#if DEBUG
    if (mDebugFill) {
        JKRFillMemory((u8*)(mHeadFreeList + 1), mHeadFreeList->size, JKRValue_DEBUGFILL_DELETE);
    }
#endif
    unlock();
}

void JKRExpHeap::do_freeTail() {
    lock();
    for (CMemBlock* block = mHeadUsedList; block != nullptr;) {
        if (block->isTempMemBlock()) {
            dispose(block->getContent(), block->size);
            CMemBlock* temp = block->mNext;
            block->free(this);
            block = temp;
        } else {
            block = block->mNext;
        }
    }
    unlock();
}

void JKRExpHeap::do_fillFreeArea() {
#if DEBUG
    lock();
    for (CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        JKRFillMemory((u8*)block->getContent(), block->size, JKRValue_DEBUGFILL_DELETE);
    }
    unlock();
#endif
}

s32 JKRExpHeap::do_changeGroupID(const u8 newGroupID) {
    lock();
    const u8 prev = mCurrentGroupId;
    mCurrentGroupId = newGroupID;
    unlock();
    return prev;
}

s32 JKRExpHeap::do_resize(void* ptr, u32 size) {
    lock();
    CMemBlock* block = CMemBlock::getHeapBlock(ptr);
    if (block == nullptr || !contains(ptr)) {
        unlock();
        return -1;
    }

    size = ALIGN_NEXT(size, MIN_ALLOC_ALIGN);
    if (size == block->size) {
        unlock();
        return size;
    }

    if (size > block->size) {
        CMemBlock* foundBlock = nullptr;
        for (CMemBlock* freeBlock = mHeadFreeList; freeBlock; freeBlock = freeBlock->mNext) {
            if (freeBlock == reinterpret_cast<CMemBlock*>(reinterpret_cast<uintptr_t>(block + 1) + block->size)) {
                foundBlock = freeBlock;
                break;
            }
        }

        if (foundBlock == nullptr) {
            unlock();
            return -1;
        }

        if (size > block->size + sizeof(CMemBlock) + foundBlock->size) {
            unlock();
            return -1;
        }

        u32 local_24 = block->size;
        removeFreeBlock(foundBlock);
        block->size += foundBlock->size + sizeof(CMemBlock);
        if (block->size - size > sizeof(CMemBlock)) {
            if (CMemBlock* newBlock = block->allocFore(size, block->mGroupId, block->mFlags, 0, 0)) {
                recycleFreeBlock(newBlock);
            }
        }
#if DEBUG
        if (mDebugFill) {
            JKRFillMemory((u8*)ptr + local_24, block->size - local_24, JKRValue_DEBUGFILL_NEW);
        }
#endif
    } else {
        if (block->size - size > sizeof(CMemBlock)) {
            if (CMemBlock* freeBlock = block->allocFore(size, block->mGroupId, block->mFlags, 0, 0)) {
                recycleFreeBlock(freeBlock);
            }
        }
    }

    unlock();
    return block->size;
}

s32 JKRExpHeap::do_getSize(void* ptr) {
    lock();
    const CMemBlock* block = CMemBlock::getHeapBlock(ptr);
    if (!block || !contains(ptr)) {
        unlock();
        return -1;
    }
    unlock();
    return block->size;
}

s32 JKRExpHeap::do_getFreeSize() {
    lock();
    s32 size = 0;
    for (const CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        if (size < static_cast<s32>(block->size)) {
            size = block->size;
        }
    }
    unlock();
    return size;
}

void* JKRExpHeap::do_getMaxFreeBlock() {
    lock();
    s32 size = 0;
    CMemBlock* res = nullptr;
    for (CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        if (size < static_cast<s32>(block->size)) {
            size = block->size;
            res = block;
        }
    }
    unlock();
    return res;
}

s32 JKRExpHeap::do_getTotalFreeSize() {
    u32 size = 0;
    lock();
    for (CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        size += block->size;
    }
    unlock();
    return size;
}

s32 JKRExpHeap::getUsedSize(const u8 groupId) const {
    lock();
    u32 size = 0;

    for (const CMemBlock* block = mHeadUsedList; block; block = block->mNext) {
        if (block->mGroupId == groupId) {
            size += block->size + sizeof(CMemBlock);
        }
    }

    unlock();
    return size;
}

s32 JKRExpHeap::getTotalUsedSize() const {
    lock();
    u32 size = 0;

    for (CMemBlock* block = mHeadUsedList; block; block = block->mNext) {
        size += block->size + sizeof(CMemBlock);
    }

    unlock();
    return size;
}

BOOL JKRExpHeap::isEmpty() const {
    lock();
    const BOOL result = !mHeadUsedList ? TRUE : FALSE;
    unlock();
    return result;
}

void JKRExpHeap::appendUsedList(JKRExpHeap::CMemBlock* newBlock) {
    if (!newBlock) {
        JUTException::panic(__FILE__, 1568, "bad appendUsedList\n");
    }

    CMemBlock* block = mTailUsedList;
    newBlock->mMagic = 'HM';

    if (block) {
        block->mNext = newBlock;
        newBlock->mPrev = block;
    } else {
        newBlock->mPrev = nullptr;
    }

    mTailUsedList = newBlock;
    if (!mHeadUsedList) {
        mHeadUsedList = newBlock;
    }
    newBlock->mNext = nullptr;
}

void JKRExpHeap::setFreeBlock(CMemBlock* block, CMemBlock* prev, CMemBlock* next) {
    if (prev == nullptr) {
        mHeadFreeList = block;
        block->mPrev = nullptr;
    } else {
        prev->mNext = block;
        block->mPrev = prev;
    }

    if (next == nullptr) {
        mTailFreeList = block;
        block->mNext = nullptr;
    } else {
        next->mPrev = block;
        block->mNext = next;
    }

    block->mMagic = 0;
}

void JKRExpHeap::removeFreeBlock(CMemBlock* block) {
    CMemBlock* prev = block->mPrev;
    CMemBlock* next = block->mNext;

    if (prev == nullptr) {
        mHeadFreeList = next;
    } else {
        prev->mNext = next;
    }

    if (next == nullptr) {
        mTailFreeList = prev;
    } else {
        next->mPrev = prev;
    }
}

void JKRExpHeap::removeUsedBlock(JKRExpHeap::CMemBlock* block) {
    CMemBlock* prev = block->mPrev;
    CMemBlock* next = block->mNext;

    if (prev == nullptr) {
        mHeadUsedList = next;
    } else {
        prev->mNext = next;
    }

    if (next == nullptr) {
        mTailUsedList = prev;
    } else {
        next->mPrev = prev;
    }
}

void JKRExpHeap::recycleFreeBlock(JKRExpHeap::CMemBlock* block) {
    JKRExpHeap::CMemBlock* newBlock = block;
    int size = block->size;
    const void* blockEnd = reinterpret_cast<u8*>(newBlock) + size;
    block->mMagic = 0;

    if ((block->mFlags & 0x7f) != 0) {
        newBlock = reinterpret_cast<CMemBlock*>(reinterpret_cast<u8*>(newBlock) - (block->mFlags & 0x7f));
        size += (block->mFlags & 0x7f);
        blockEnd = reinterpret_cast<u8*>(newBlock) + size;
        newBlock->mGroupId = 0;
        newBlock->mFlags = 0;
        newBlock->size = size;
    }
#if DEBUG
    u8* local_24 = (u8*)(newBlock + 1);
    u32 local_28 = newBlock->size;
    if (mDebugFill) {
        JKRFillMemory(local_24, local_28, JKRValue_DEBUGFILL_DELETE);
    }
#endif
    if (!mHeadFreeList) {
        newBlock->initiate(nullptr, nullptr, size, 0, 0);
        mHeadFreeList = newBlock;
        mTailFreeList = newBlock;
        setFreeBlock(newBlock, nullptr, nullptr);
        return;
    }

    if (mHeadFreeList >= blockEnd) {
        newBlock->initiate(nullptr, nullptr, size, 0, 0);
        setFreeBlock(newBlock, nullptr, mHeadFreeList);
        joinTwoBlocks(newBlock);
        return;
    }

    if (mTailFreeList <= newBlock) {
        newBlock->initiate(nullptr, nullptr, size, 0, 0);
        setFreeBlock(newBlock, mTailFreeList, nullptr);
        joinTwoBlocks(newBlock->mPrev);
        return;
    }

    for (CMemBlock* freeBlock = mHeadFreeList; freeBlock; freeBlock = freeBlock->mNext) {
        if (freeBlock >= newBlock || newBlock >= freeBlock->mNext) {
            continue;
        }
        newBlock->mNext = freeBlock->mNext;
        newBlock->mPrev = freeBlock;
        freeBlock->mNext = newBlock;
        newBlock->mNext->mPrev = newBlock;
        newBlock->mGroupId = 0;
        joinTwoBlocks(newBlock);
        joinTwoBlocks(freeBlock);
        return;
    }
}

void JKRExpHeap::joinTwoBlocks(CMemBlock* block) {
    const uintptr_t endAddr = reinterpret_cast<uintptr_t>(block + 1) + block->size;
    CMemBlock* next = block->mNext;
    const uintptr_t nextAddr = reinterpret_cast<uintptr_t>(next) - (next->mFlags & 0x7f);
    if (endAddr > nextAddr) {
        JUTWarningConsole_f(":::Heap may be broken. (block = %x)", block);
        OS_REPORT(":::block = %x\n", block);
        OS_REPORT(":::joinTwoBlocks [%x %x %x][%x %x %x]\n", block, block->mFlags, block->size, block->mNext, block->mNext->mFlags, block->mNext->size);
        OS_REPORT(":::: endAddr = %p\n", (void*)endAddr);
        OS_REPORT(":::: nextAddr = %p\n", (void*)nextAddr);
        JKRHeap* heap = JKRGetCurrentHeap();
        heap->dump();
        JUTException::panic(__FILE__, 1820, "Bad Block\n");
    }

    if (endAddr == nextAddr) {
        block->size = next->size + sizeof(CMemBlock) + (next->mFlags & 0x7f) + block->size;
        CMemBlock* local_30 = next->mNext;
#if DEBUG
        u8* local_34 = (u8*)next;
        u32 local_38 = sizeof(CMemBlock);
        if (mDebugFill) {
            JKRFillMemory(local_34, local_38, JKRValue_DEBUGFILL_DELETE);
        }
#endif
        setFreeBlock(block, block->mPrev, local_30);
    }
}

bool JKRExpHeap::check() {
    lock();
    int totalBytes = 0;
    bool ok = true;
    for (CMemBlock* block = mHeadUsedList; block; block = block->mNext) {
        if (block->mMagic != 'HM') {
            ok = false;
            JUTWarningConsole_f(":::addr %08x: bad heap signature. (%c%c)\n", block,
                                JSUHiByte(block->mMagic), JSULoByte(block->mMagic));
        }

        if (block->mNext) {
            if (block->mNext->mMagic != 'HM') {
                ok = false;
                JUTWarningConsole_f(":::addr %08x: bad next pointer (%08x)\nabort\n", block,
                                    block->mNext);
                break;
            }
            if (block->mNext->mPrev != block) {
                ok = false;
                JUTWarningConsole_f(":::addr %08x: bad previous pointer (%08x)\n", block->mNext,
                                    block->mNext->mPrev);
            }
        } else {
            if (mTailUsedList != block) {
                ok = false;
                JUTWarningConsole_f(":::addr %08x: bad used list(REV) (%08x)\n", block,
                                    mTailUsedList);
            }
        }
        totalBytes += sizeof(CMemBlock) + block->size + block->getAlignment();
    }

    for (CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        totalBytes += block->size + sizeof(CMemBlock);
        if (block->mNext) {
            if (block->mNext->mPrev != block) {
                ok = false;
                JUTWarningConsole_f(":::addr %08x: bad previous pointer (%08x)\n", block->mNext,
                                    block->mNext->mPrev);
            }

            if (reinterpret_cast<uintptr_t>(block) + block->size + sizeof(CMemBlock) > reinterpret_cast<uintptr_t>(block->mNext)) {
                ok = false;
                JUTWarningConsole_f(":::addr %08x: bad block size (%08x)\n", block, block->size);
            }
        } else {
            if (mTailFreeList != block) {
                ok = false;
                JUTWarningConsole_f(":::addr %08x: bad used list(REV) (%08x)\n", block,
                                    mTailFreeList);
            }
        }
#if DEBUG
        if (mCheckMemoryFilled) {
            u8* local_34 = (u8*)block->getContent();
            u32 local_38 = block->size;
            ok = JKRHeap::checkMemoryFilled(local_34, local_38, JKRValue_DEBUGFILL_DELETE);
        }
#endif
    }

    if (totalBytes != mSize) {
        ok = false;
        JUTWarningConsole_f(":::bad total memory block size (%08X, %08X)\n", mSize, totalBytes);
    }

    if (!ok) {
        JUTWarningConsole(":::there is some error in this heap!\n");
    }

    unlock();
    return ok;
}

bool JKRExpHeap::dump() {
    lock();
    const bool result = check();
    u32 usedBytes = 0;
    u32 usedCount = 0;
    u32 freeCount = 0;

    JUTReportConsole(" attr  address:   size    gid aln   prev_ptr next_ptr\n");
    JUTReportConsole("(Used Blocks)\n");
    if (!mHeadUsedList) {
        JUTReportConsole(" NONE\n");
    }

    for (CMemBlock* block = mHeadUsedList; block; block = block->mNext) {
        if (block->mMagic != 'HM') {
            JUTReportConsole_f("xxxxx %08x: --------  --- ---  (-------- --------)\nabort\n",
                               block);
            break;
        }

        JUTReportConsole_f("%s %08x: %08x  %3d %3d  (%08x %08x)\n",
                           block->isTempMemBlock() ? " temp" : "alloc", block->getContent(),
                           block->size, block->mGroupId, block->getAlignment(), block->mPrev,
                           block->mNext);
        usedBytes += sizeof(CMemBlock) + block->size + block->getAlignment();
        usedCount++;
    }

    JUTReportConsole("(Free Blocks)\n");
    if (!mHeadFreeList) {
        JUTReportConsole(" NONE\n");
    }

    for (const CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        JUTReportConsole_f("%s %08x: %08x  %3d %3d  (%08x %08x)\n", " free", block->getContent(),
                           block->size, block->mGroupId, block->getAlignment(), block->mPrev,
                           block->mNext);
        freeCount++;
    }

    JUTReportConsole_f("%d / %d bytes (%6.2f%%) used (U:%d F:%d)\n", usedBytes, mSize, (f32(usedBytes) / f32(mSize)) * 100.0f,
                       usedCount, freeCount);
    unlock();
    return result;
}

bool JKRExpHeap::dump_sort() {
    lock();
    const bool result = check();
    u32 usedBytes = 0;
    u32 usedCount = 0;
    u32 freeCount = 0;
    JUTReportConsole(" attr  address:   size    gid aln   prev_ptr next_ptr\n");
    JUTReportConsole("(Used Blocks)\n");

    if (mHeadUsedList == nullptr) {
        JUTReportConsole(" NONE\n");
    } else {
        CMemBlock* var1 = nullptr;
        while (true) {
            auto* block = reinterpret_cast<CMemBlock*>(0xffffffff);
            for (CMemBlock* iterBlock = mHeadUsedList; iterBlock; iterBlock = iterBlock->mNext) {
                if (var1 < iterBlock && iterBlock < block) {
                    block = iterBlock;
                }
            }

            if (reinterpret_cast<uintptr_t>(block) == 0xffffffff) {
                break;
            }

            if (block->mMagic != 'HM') {
                JUTReportConsole_f("xxxxx %08x: --------  --- ---  (-------- --------)\nabort\n",
                                   var1);
                break;
            }

            JUTReportConsole_f("%s %08x: %08x  %3d %3d  (%08x %08x)\n", block->isTempMemBlock() ? " temp" : "alloc", block->getContent(), block->size,
                               block->mGroupId, block->getAlignment(), block->mPrev, block->mNext);
            usedBytes += sizeof(CMemBlock) + block->size + block->getAlignment();
            usedCount++;
            var1 = block;
        }
    }

    JUTReportConsole("(Free Blocks)\n");
    if (mHeadFreeList == nullptr) {
        JUTReportConsole(" NONE\n");
    }

    for (const CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        JUTReportConsole_f("%s %08x: %08x  %3d %3d  (%08x %08x)\n", " free", block->getContent(),
                           block->size, block->mGroupId, block->getAlignment(), block->mPrev,
                           block->mNext);
        freeCount++;
    }

    JUTReportConsole_f("%d / %d bytes (%6.2f%%) used (U:%d F:%d)\n", usedBytes, mSize, (static_cast<f32>(usedBytes) / static_cast<f32>(mSize)) * 100.0f,
                       usedCount, freeCount);
    unlock();
    return result;
}

void JKRExpHeap::CMemBlock::initiate(JKRExpHeap::CMemBlock* prev, JKRExpHeap::CMemBlock* next,
                                     const u32 size, const u8 groupId, const u8 alignment) {
    mMagic = 'HM';
    mFlags = alignment;
    mGroupId = groupId;
    this->size = size;
    mPrev = prev;
    mNext = next;
}

JKRExpHeap::CMemBlock* JKRExpHeap::CMemBlock::allocFore(const u32 size1, const u8 groupId1,
                                                        const u8 alignment1, const u8 groupId2,
                                                        const u8 alignment2) {
    CMemBlock* block = nullptr;
    mGroupId = groupId1;
    mFlags = alignment1;
    if (size >= size1 + sizeof(CMemBlock)) {
        block = reinterpret_cast<CMemBlock*>(size1 + reinterpret_cast<uintptr_t>(this));
        block[1].mGroupId = groupId2;
        block[1].mFlags = alignment2;
        block[1].size = size - (size1 + sizeof(CMemBlock));
        size = size1;
        block = block + 1;
    }
    return block;
}

JKRExpHeap::CMemBlock* JKRExpHeap::CMemBlock::allocBack(const u32 size1, const u8 groupId1,
                                                        const u8 alignment1, const u8 groupId2,
                                                        const u8 alignment2) {
    CMemBlock* newBlock = nullptr;
    if (size >= size1 + sizeof(CMemBlock)) {
        newBlock = reinterpret_cast<CMemBlock*>(reinterpret_cast<uintptr_t>(this) + size - size1);
        newBlock->mGroupId = groupId2;
        newBlock->mFlags = alignment2 | 0x80;
        newBlock->size = size1;
        mGroupId = groupId1;
        mFlags = alignment1;
        this->size -= size1 + sizeof(CMemBlock);
    } else {
        mGroupId = groupId2;
        mFlags = 0x80;
    }
    return newBlock;
}

int JKRExpHeap::CMemBlock::free(JKRExpHeap* heap) {
    heap->removeUsedBlock(this);
    heap->recycleFreeBlock(this);
    return 0;
}

JKRExpHeap::CMemBlock* JKRExpHeap::CMemBlock::getHeapBlock(void* ptr) {
    if (ptr) {
        if (CMemBlock* block = static_cast<CMemBlock*>(ptr) - 1; block->isValid()) {
            return block;
        }
    }
    return nullptr;
}

static void dummy2() {
    DEAD_STRING("+---------------JKRExpHeap\n");
    DEAD_STRING("|         Align Group  size    ( prev , next )\n");
    DEAD_STRING("| ---- FreeFirst\n");
    DEAD_STRING("| %08x  ");
    DEAD_STRING("%2x  %3d  %6x  (%08x %08x)\n");
    DEAD_STRING("| ---- FreeLast\n");
    DEAD_STRING("| ---- UsedFirst\n");
    DEAD_STRING("| ---- UsedLast\n");
    DEAD_STRING("+---------------End\n");
}

void JKRExpHeap::state_register(JKRHeap::TState* p, const u32 id) const {
    JUT_ASSERT(2433, p != nullptr);
    JUT_ASSERT(2434, p->getHeap() == this);
    void* _r24 = getState_(p);
    const u32 r25 = id;
    setState_u32ID_(p, id);
    if (id <= 0xff) {
        setState_uUsedSize_(p, getUsedSize(r25));
    } else {
        setState_uUsedSize_(p, getUsedSize_(const_cast<JKRExpHeap*>(this)));
    }

    u32 checkCode = 0;
    for (CMemBlock* block = mHeadUsedList; block; block = block->mNext) {
        if (id <= 0xff) {
            if (block->mGroupId == id) {
                checkCode += reinterpret_cast<uintptr_t>(block) * 3;
            }
        } else {
            checkCode += reinterpret_cast<uintptr_t>(block) * 3;
        }
    }
    setState_u32CheckCode_(p, checkCode);
}

bool JKRExpHeap::state_compare(JKRHeap::TState const& r1, JKRHeap::TState const& r2) const {
    JUT_ASSERT(2481, r1.getHeap() == r2.getHeap());
    bool result = true;
    if (r1.getCheckCode() != r2.getCheckCode()) {
        result = false;
    }

    if (r1.getUsedSize() != r2.getUsedSize()) {
        result = false;
    }

    return result;
}

u32 JKRExpHeap::getHeapType() {
    return 'EXPH';
}

u8 JKRExpHeap::do_getCurrentGroupId() {
    return mCurrentGroupId;
}
