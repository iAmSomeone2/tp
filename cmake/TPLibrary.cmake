# Helpers for declaring the project's libraries.
include_guard(GLOBAL)

# tp_add_library(<name> SOURCES <file>...)
#
# Creates the static library `tp_<name>` (alias `tp::<name>`). Source paths are relative to
# the calling directory. Every library gets the common configuration and the public headers of
# JSystem and the Dolphin SDK; warning flags stay private.
function(tp_add_library name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES")
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "tp_add_library(${name}): no SOURCES given")
    endif()

    set(target tp_${name})
    add_library(${target} STATIC)
    add_library(tp::${name} ALIAS ${target})

    target_sources(${target} PRIVATE ${ARG_SOURCES})
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    target_link_libraries(${target}
        PUBLIC tp::config tp::JSystem_headers tp::dolphin
        PRIVATE tp::warnings
    )

    set_property(GLOBAL APPEND PROPERTY TP_LIBRARIES ${target})
endfunction()

# tp_add_engine_target()
#
# Defines `tp::engine`, an interface library that links every tp_add_library() target. The
# libraries reference each other freely (framework <-> game logic <-> actors), so on linkers
# that support it they are placed in a rescanned group.
function(tp_add_engine_target)
    get_property(libraries GLOBAL PROPERTY TP_LIBRARIES)

    add_library(tp_engine INTERFACE)
    add_library(tp::engine ALIAS tp_engine)

    if(CMAKE_CXX_LINK_GROUP_USING_RESCAN_SUPPORTED OR CMAKE_LINK_GROUP_USING_RESCAN_SUPPORTED)
        string(REPLACE ";" "," libraries_csv "${libraries}")
        target_link_libraries(tp_engine INTERFACE "$<LINK_GROUP:RESCAN,${libraries_csv}>")
    else()
        target_link_libraries(tp_engine INTERFACE ${libraries})
    endif()
endfunction()

# tp_add_maintenance_targets()
#
# `tp_update_sources` regenerates the sources.cmake lists from configure.py and the splits;
# `tp_check_sources` fails if they are stale. Both need Python 3 and are skipped without it.
function(tp_add_maintenance_targets)
    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(NOT Python3_Interpreter_FOUND)
        return()
    endif()

    set(script "${PROJECT_SOURCE_DIR}/tools/utilities/gen_cmake_sources.py")
    add_custom_target(tp_update_sources
        COMMAND Python3::Interpreter "${script}"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Regenerating sources.cmake files"
        VERBATIM
    )
    add_custom_target(tp_check_sources
        COMMAND Python3::Interpreter "${script}" --check
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Checking sources.cmake files are up to date"
        VERBATIM
    )
endfunction()
