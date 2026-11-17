# Tests for JKernel: archive mount sharing, the decompression thread, the async-load callback and
# JKRExpHeap block merging, JKRHeap as a std::pmr::memory_resource, the heaps' minimum alignment, keeping the current heap on the main thread and finding a heap by address.
#
# These link the real JKernel code (see tests/JSystem/CMakeLists.txt).
tp_add_test(JKernel_JKRArchivePub_test
    SOURCES JKRArchivePub_test.cpp
    LIBRARIES tp_test_jsystem_libs
    LENIENT_LINK
)

tp_add_test(JKernel_JKRDecomp_test
    SOURCES JKRDecomp_test.cpp
    LIBRARIES tp_test_jsystem_libs
    LENIENT_LINK
)

tp_add_test(JKernel_JKRDvdAramRipper_test
    SOURCES JKRDvdAramRipper_test.cpp
    LIBRARIES tp_test_jsystem_libs
    LENIENT_LINK
)

tp_add_test(JKernel_JKRExpHeap_test
    SOURCES JKRExpHeap_test.cpp
    LIBRARIES tp_test_jsystem_libs
    LENIENT_LINK
)

tp_add_test(JKernel_JKRHeap_resource_test
    SOURCES JKRHeap_resource_test.cpp
    LIBRARIES tp_test_jsystem_libs nf::memory
    LENIENT_LINK
)

tp_add_test(JKernel_JKRHeap_alignment_test
    SOURCES JKRHeap_alignment_test.cpp
    LIBRARIES tp_test_jsystem_libs
    LENIENT_LINK
)

tp_add_test(JKernel_JKRHeap_thread_test
    SOURCES JKRHeap_thread_test.cpp
    LIBRARIES tp_test_jsystem_libs
    LENIENT_LINK
)

tp_add_test(JKernel_JKRHeap_find_test
    SOURCES JKRHeap_find_test.cpp
    LIBRARIES tp_test_jsystem_libs
    LENIENT_LINK
)
