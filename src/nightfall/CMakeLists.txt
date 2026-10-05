# Minimal compatibility fixes allowing engine code to be built on Clang or GCC

# Host definitions of globals that the SDK headers only declare (see platform/os_globals.cpp).
# Linked by tp::engine and by the tests; a final executable gets it through tp::engine.
add_library(nf_platform STATIC platform/os_globals.cpp)
add_library(nf::platform ALIAS nf_platform)
set_target_properties(nf_platform PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(nf_platform
    PUBLIC tp::config tp::dolphin
    PRIVATE tp::warnings
)
set_property(GLOBAL APPEND PROPERTY TP_LIBRARIES nf_platform)
