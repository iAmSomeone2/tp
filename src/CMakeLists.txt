# Game code. The libraries mirror configure.py's, and their sources are listed in the
# sources.cmake next to each CMakeLists.txt.
include(${CMAKE_CURRENT_LIST_DIR}/sources.cmake)

add_subdirectory(nightfall)

# Process framework: f_ap / f_op / f_pc.
tp_add_library(framework SOURCES ${TP_FRAMEWORK_SOURCES})
# The table of every process profile (a standalone REL in the original game).
tp_add_library(f_pc_profile_lst SOURCES ${TP_PROFILE_LIST_SOURCES})
tp_add_library(DynamicLink SOURCES ${TP_DYNAMICLINK_SOURCES})

add_subdirectory(m_Do)
add_subdirectory(c)
add_subdirectory(SSystem)
add_subdirectory(Z2AudioLib)
add_subdirectory(d)
