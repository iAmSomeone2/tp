# JSystem: Nintendo's middleware (J3D/J2D, JKernel, JAudio2, JStudio, ...). One static library
# per module, as in configure.py. Source lists come from sources.cmake.
include(${CMAKE_CURRENT_LIST_DIR}/sources.cmake)

add_library(tp_JSystem_headers INTERFACE)
add_library(tp::JSystem_headers ALIAS tp_JSystem_headers)
target_include_directories(tp_JSystem_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

foreach(module IN LISTS TP_JSYSTEM_MODULES)
    string(TOUPPER "${module}" module_upper)
    tp_add_library(${module} SOURCES ${TP_JSYSTEM_${module_upper}_SOURCES})
endforeach()
