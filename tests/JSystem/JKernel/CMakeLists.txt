# Tests for JKernel: archive mount sharing, the decompression thread and the async-load callback.
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
