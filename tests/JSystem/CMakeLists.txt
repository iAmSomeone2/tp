# Tests for libs/JSystem, one directory per JSystem module (as in libs/JSystem/src).
#
# The tests link the real JSystem libraries, not copies of the sources. Only the libraries they
# need are linked (not tp::engine), so they build even where unrelated engine files cannot, for
# example the ones that need generated assets. The libraries refer to each other, so on linkers
# that support it they are put in a rescanned group, as tp::engine does. See tests/CMakeLists.txt
# for LENIENT_LINK and the stand-ins in tests/support/.
set(_tp_test_jsystem_libs tp_JAudio2 tp_JKernel tp_JMessage tp_JSupport tp_JUtility tp_JGadget)
add_library(tp_test_jsystem_libs INTERFACE)
if (CMAKE_CXX_LINK_GROUP_USING_RESCAN_SUPPORTED OR CMAKE_LINK_GROUP_USING_RESCAN_SUPPORTED)
    string(REPLACE ";" "," _tp_test_jsystem_libs_csv "${_tp_test_jsystem_libs}")
    target_link_libraries(tp_test_jsystem_libs INTERFACE
        "$<LINK_GROUP:RESCAN,${_tp_test_jsystem_libs_csv}>")
else ()
    target_link_libraries(tp_test_jsystem_libs INTERFACE ${_tp_test_jsystem_libs})
endif ()

add_subdirectory(JAudio2)
add_subdirectory(JKernel)
add_subdirectory(JMessage)
add_subdirectory(JUtility)
