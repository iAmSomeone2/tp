# Machine-specific glue ("machine" in configure.py).
include(${CMAKE_CURRENT_LIST_DIR}/sources.cmake)

tp_add_library(machine SOURCES ${TP_MACHINE_SOURCES})
