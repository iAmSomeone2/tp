# Machine-specific glue ("machine" in configure.py).
include(${CMAKE_CURRENT_LIST_DIR}/sources.cmake)

# m_Do_main.cpp holds main(), so it is the executable's own source instead of part of the
# library. (sources.cmake is generated and still lists it with the rest of "machine".)
set(TP_MAIN_SOURCE m_Do_main.cpp)
list(REMOVE_ITEM TP_MACHINE_SOURCES ${TP_MAIN_SOURCE})

tp_add_library(machine SOURCES ${TP_MACHINE_SOURCES})

# The game executable. tp::engine links every library in a rescanned group, so the libraries'
# references back into m_Do (and each other) resolve regardless of order. It cannot link
# successfully until the SDK implementation exists; it is a target now so that tools that follow
# the target graph see main() and what it reaches.
add_executable(tp ${TP_MAIN_SOURCE})
target_link_libraries(tp
    PRIVATE
        tp::config
        tp::engine
        tp::warnings
)
