# Generation of the `assets/*.h` headers from the user's disc image.
include_guard(GLOBAL)

set(TP_GENERATE_ASSETS AUTO CACHE STRING
    "Add a build step that generates assets/*.h from the disc image in orig/<TP_VERSION>: AUTO (when an image is found), ON (always; fails without one) or OFF")
set_property(CACHE TP_GENERATE_ASSETS PROPERTY STRINGS AUTO ON OFF)
set(TP_DTK_PATH "" CACHE FILEPATH
    "decomp-toolkit binary used to generate asset headers (default: downloaded to build/tools/)")

# tp_add_asset_target()
#
# Defines `tp_generate_assets`, which runs tools/utilities/gen_asset_headers.py (decomp-toolkit's
# `dol split` plus the matDL converter, the same steps configure.py/ninja performs) and writes
# the headers into build/<TP_VERSION>/include/assets, which is TP_GENERATED_INCLUDE_DIR's
# default. The libraries that hold the translation units which include those headers depend on
# it, so a normal build generates them first; the step reruns only when the disc image,
# config/<ver>/config.yml or the scripts change. Call this after the libraries are defined.
function(tp_add_asset_target)
    if(TP_GENERATE_ASSETS STREQUAL "OFF")
        return()
    endif()

    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(NOT Python3_Interpreter_FOUND)
        if(TP_GENERATE_ASSETS STREQUAL "ON")
            message(FATAL_ERROR "TP_GENERATE_ASSETS=ON needs a Python 3 interpreter")
        endif()
        return()
    endif()

    set(disc_dir "${PROJECT_SOURCE_DIR}/orig/${TP_VERSION}")
    file(GLOB disc_files CONFIGURE_DEPENDS LIST_DIRECTORIES false "${disc_dir}/*")
    list(FILTER disc_files EXCLUDE REGEX "/\\.[^/]*$")
    if(NOT disc_files)
        if(TP_GENERATE_ASSETS STREQUAL "ON")
            message(FATAL_ERROR
                "TP_GENERATE_ASSETS=ON but there is no disc image in ${disc_dir} (see README.md)")
        endif()
        message(STATUS "TP: no disc image in ${disc_dir}; not adding the asset header target")
        return()
    endif()

    if(NOT TP_GENERATED_INCLUDE_DIR STREQUAL "${PROJECT_SOURCE_DIR}/build/${TP_VERSION}/include")
        message(WARNING
            "TP: the asset script writes to build/${TP_VERSION}/include, but "
            "TP_GENERATED_INCLUDE_DIR is ${TP_GENERATED_INCLUDE_DIR}; the generated headers will "
            "not be found")
    endif()

    set(script "${PROJECT_SOURCE_DIR}/tools/utilities/gen_asset_headers.py")
    set(stamp "${PROJECT_BINARY_DIR}/tp_assets/${TP_VERSION}.stamp")
    set(dtk_args "")
    if(TP_DTK_PATH)
        set(dtk_args --dtk "${TP_DTK_PATH}")
    endif()

    add_custom_command(
        OUTPUT "${stamp}"
        COMMAND Python3::Interpreter "${script}"
                --version "${TP_VERSION}"
                --build-dir "${PROJECT_SOURCE_DIR}/build"
                --stamp "${stamp}"
                ${dtk_args}
        DEPENDS
            "${script}"
            "${PROJECT_SOURCE_DIR}/tools/converters/matDL_dis.py"
            "${PROJECT_SOURCE_DIR}/config/${TP_VERSION}/config.yml"
            ${disc_files}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Generating asset headers from the ${TP_VERSION} disc image"
        VERBATIM
    )
    add_custom_target(tp_generate_assets DEPENDS "${stamp}")

    # The libraries that contain d_a_grass, d_a_mant, d_a_player and m_Do_ext.
    foreach(library IN ITEMS tp_machine tp_dolzel tp_actors)
        if(TARGET ${library})
            add_dependencies(${library} tp_generate_assets)
        endif()
    endforeach()
endfunction()
