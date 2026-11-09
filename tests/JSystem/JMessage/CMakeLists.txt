# Tests for JMessage: message-ID lookup in a message file's MID1 block.
tp_add_test(JMessage_resource_test
    SOURCES resource_test.cpp
    LIBRARIES tp_test_jsystem_libs
    LENIENT_LINK
)
