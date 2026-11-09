# Tests for JAudio2: the JASHeap sub-allocator and the ARAM chunk manager built on it.
tp_add_test(JAudio2_JASHeapCtrl_test
    SOURCES JASHeapCtrl_test.cpp
    LIBRARIES tp_test_jsystem_libs
    LENIENT_LINK
)

tp_add_test(JAudio2_JAUStreamAramMgr_test
    SOURCES JAUStreamAramMgr_test.cpp
    LIBRARIES tp_test_jsystem_libs
    LENIENT_LINK
)
