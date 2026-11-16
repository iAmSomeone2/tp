#pragma once

#include <cstdint>
#include <memory_resource>
#include <new>
#include <optional>
#include <thread>
#include <os.h>
#include "JSystem/JKernel/JKRDisposer.h"
#include "global.h"

class JKRHeap;
typedef void (*JKRErrorHandler)(void*, u32, int);

extern u8 JKRValue_DEBUGFILL_NOTUSE;
extern u8 JKRValue_DEBUGFILL_NEW;
extern u8 JKRValue_DEBUGFILL_DELETE;

extern s32 fillcheck_dispcount;
extern bool data_8074A8D0_debug;

/**
 * Minimum allocation alignment
 *
 * Block sizes must all be multiples of this value.
 */
static constexpr size_t MIN_ALLOC_ALIGN = alignof(std::max_align_t);

/**
 * @ingroup jsystem-jkernel
 * 
 */
class JKRHeap : public JKRDisposer, public std::pmr::memory_resource {
public:
    typedef void (*JKRAllocCallback)(u32, int, JKRHeap*, void*);
    typedef void (*JKRFreeCallback)(void*, JKRHeap*);

    class TState {
    public:
        /* 0x00 */ u32 mUsedSize;
        /* 0x04 */ u32 mCheckCode;
        /* 0x08 */ u32 mBuf;
        /* 0x0C */ u32 field_0xc;
        /* 0x10 */ JKRHeap* mHeap;
        /* 0x14 */ u32 mId;

    public:
        u32 getUsedSize() const;
        u32 getCheckCode() const;
        JKRHeap* getHeap() const;
        u32 getId() const;
    };

public:
    JKRHeap(u32 size, JKRHeap* parent, bool errorFlag);
    JKRHeap(void* data, u32 size, JKRHeap* parent, bool errorFlag);
    virtual ~JKRHeap();

    JKRHeap* becomeSystemHeap();
    JKRHeap* becomeCurrentHeap();
    void destroy();

    void* alloc(u32 size, int alignment);
    void free(void* ptr);
    void freeAll();
    void freeTail();
    s32 resize(void* ptr, u32 size);
    s32 getSize(void* ptr);
    s32 getFreeSize();
    void* getMaxFreeBlock();
    s32 getTotalFreeSize();
    s32 changeGroupID(u8 newGroupId);
    u8 getCurrentGroupId();
    u32 getMaxAllocatableSize(int alignment);

    JKRHeap* find(void* ptr) const;
    JKRHeap* findAllHeap(void* ptr) const;

    void dispose_subroutine(uintptr_t start, uintptr_t end) const;
    bool dispose(void* ptr, u32 size) const;
    void dispose(void* begin, void* end) const;
    void dispose() const;

    bool setErrorFlag(bool errorFlag);
    bool isSubHeap(JKRHeap* heap) const;
    static void* getAltAramStartAdr();

    /* vt[03] */ virtual void callAllDisposer();
    /* vt[04] */ virtual u32 getHeapType() = 0;
    /* vt[05] */ virtual bool check() = 0;
    /* vt[06] */ virtual bool dump_sort();
    /* vt[07] */ virtual bool dump() = 0;
    /* vt[08] */ virtual void do_destroy() = 0;
    /* vt[09] */ virtual void* do_alloc(u32 size, int alignment) = 0;
    /* vt[10] */ virtual void do_free(void* ptr) = 0;
    /* vt[11] */ virtual void do_freeAll() = 0;
    /* vt[12] */ virtual void do_freeTail() = 0;
    /* vt[13] */ virtual void do_fillFreeArea() = 0;
    /* vt[14] */ virtual s32 do_resize(void* ptr, u32 size) = 0;
    /* vt[15] */ virtual s32 do_getSize(void* ptr) = 0;
    /* vt[16] */ virtual s32 do_getFreeSize() = 0;
    /* vt[17] */ virtual void* do_getMaxFreeBlock() = 0;
    /* vt[18] */ virtual s32 do_getTotalFreeSize() = 0;
    /* vt[19] */ virtual s32 do_changeGroupID(u8 newGroupID);
    /* vt[20] */ virtual u8 do_getCurrentGroupId();
    /* vt[21] */ virtual void state_register(JKRHeap::TState* p, u32 id) const;
    /* vt[22] */ virtual bool state_compare(JKRHeap::TState const& r1, JKRHeap::TState const& r2) const;
    /* vt[23] */ virtual void state_dump(JKRHeap::TState const& p) const;

    void setDebugFill(bool debugFill);
    bool getDebugFill() const;
    void* getStartAddr();
    void* getEndAddr();
    u32 getSize() const;
    bool getErrorFlag() const;
    void callErrorHandler(void* heap, u32 size, int alignment);

    [[nodiscard]] JKRHeap* getParent() const;

    JSUTree<JKRHeap>& getHeapTree();
    void appendDisposer(JKRDisposer* disposer);
    void removeDisposer(JKRDisposer* disposer);
    void lock() const;
    void unlock() const;
    u32 getHeapSize();

protected:
    /* 0x00 */  // vtable
    /* 0x04 */  // JKRDisposer
    /* 0x18 */ OSMutex mMutex;
    /* 0x30 */ u8* mStart;
    /* 0x34 */ u8* mEnd;
    /* 0x38 */ u32 mSize;
    /* 0x3C */ bool mDebugFill;
    /* 0x3D */ bool mCheckMemoryFilled;
    /* 0x3E */ u8 mAllocationMode;  // EAllocMode?
    /* 0x3F */ u8 mGroupId;
    /* 0x40 */ JSUTree<JKRHeap> mChildTree;
    /* 0x5C */ JSUList<JKRDisposer> mDisposerList;
    /* 0x68 */ bool mErrorFlag;
    /* 0x69 */ bool mInitFlag;
    std::optional<std::thread::id> mHomeThread;

public:
    static bool initArena(char** memory, u32* size, int maxHeaps);
    static bool initArena2(char** memory, u32* size, int maxHeaps);
    static void* alloc(u32 size, int alignment, JKRHeap* heap);
    static void free(void* ptr, JKRHeap* heap);
    static s32 resize(void* ptr, u32 size, JKRHeap* heap);
    static s32 getSize(void* ptr, JKRHeap* heap);
    static JKRHeap* findFromRoot(void* ptr);

    static void copyMemory(void* dst, void* src, u32 size);
    static void fillMemory(void* dst, u32 size, u8 value);  // NOTE: never used
    static bool checkMemoryFilled(void* src, u32 size, u8 value);

    static JKRErrorHandler setErrorHandler(JKRErrorHandler errorHandler);
    static void fillMemory(u8*, u32, u8);
    static bool checkMemoryFilled(u8*, u32, u8);

    static bool isDefaultDebugFill();
    static void setDefaultDebugFill(bool status);
    static void* getCodeStart();
    static void* getCodeEnd();
    static void* getUserRamStart();
    static void* getUserRamEnd();
    static u32 getMemorySize();
    static JKRHeap* getRootHeap();

#if PLATFORM_WII || PLATFORM_SHIELD
    static JKRHeap* getRootHeap2();
#endif

    static JKRHeap* getSystemHeap();
    static JKRHeap* getCurrentHeap();
    static void setSystemHeap(JKRHeap* heap);
    static void setCurrentHeap(JKRHeap* heap);

    static void setState_u32ID_(TState* state, u32 id);
    static void setState_uUsedSize_(TState* state, u32 usedSize);
    static void setState_u32CheckCode_(TState* state, u32 checkCode);
    static void* getState_buf_(TState* state);

    static void* mCodeStart;
    static void* mCodeEnd;
    static void* mUserRamStart;
    static void* mUserRamEnd;
    static u32 mMemorySize;
    static JKRAllocCallback sAllocCallback;
    static JKRFreeCallback sFreeCallback;

    static bool sDefaultFillFlag;

    static JKRHeap* sRootHeap;

#if PLATFORM_WII || PLATFORM_SHIELD
    static JKRHeap* sRootHeap2;
#endif

    static JKRHeap* sSystemHeap;
    static JKRHeap* sCurrentHeap;

    static JKRErrorHandler mErrorHandler;

private:
    void* do_allocate(std::size_t _bytes, std::size_t _alignment) override;
    void do_deallocate(void* _ptr, std::size_t _bytes, std::size_t _alignment) noexcept override;
    [[nodiscard]] bool do_is_equal(const memory_resource& _other) const noexcept override;
    /**
     * Recursively work through the heap tree until the root heap node is located.
     * @param heap node to begin searching from
     * @return a pointer to the located root heap node
     */
    [[nodiscard]] static const JKRHeap* getRoot(const JKRHeap* heap) noexcept;
    /**
     * Check whether the current thread is the same one that this heap was allocated from
     * @return `true` if this is the 'home' thread; otherwise, `false`
     */
    [[nodiscard]] bool current_thread_is_home() const noexcept;
};

void* operator new(size_t size);
void* operator new(size_t size, int alignment);
void* operator new(size_t size, JKRHeap* heap, int alignment);

void* operator new[](size_t size);
void* operator new[](size_t size, int alignment);
void* operator new[](size_t size, JKRHeap* heap, int alignment);

void operator delete(void* ptr);
void operator delete[](void* ptr);

void JKRDefaultMemoryErrorRoutine(void* heap, u32 size, int alignment);

void* JKRAllocFromHeap(JKRHeap* heap, u32 size, int alignment);
void* JKRAllocFromSysHeap(u32 size, int alignment);
void JKRFreeToHeap(JKRHeap* heap, void* ptr);
void JKRFreeToSysHeap(void* ptr);
void JKRFree(void* ptr);
void JKRFillMemory(u8* dst, u32 size, u8 val);
JKRHeap* JKRGetSystemHeap();
JKRHeap* JKRGetCurrentHeap();
JKRHeap* JKRSetCurrentHeap(JKRHeap* heap);
u32 JKRGetMemBlockSize(JKRHeap* heap, void* block);
u32 JKRGetFreeSize(JKRHeap* heap);
void* JKRAlloc(u32 size, int alignment);
s32 JKRResizeMemBlock(JKRHeap* heap, void* ptr, u32 size);
JKRHeap* JKRFindHeap(void* ptr);
JKRHeap* JKRGetRootHeap();
JKRErrorHandler JKRSetErrorHandler(JKRErrorHandler errorHandler);
bool JKRSetErrorFlag(JKRHeap* heap, bool flag);
#if PLATFORM_WII || PLATFORM_SHIELD
JKRHeap* JKRGetRootHeap2();
#endif

#if DEBUG
void JKRSetDebugFillNotuse(u8 status);
void JKRSetDebugFillNew(u8 status);
void JKRSetDebugFillDelete(u8 status);
#endif
