# Refuses toolchains this project does not support, with an actionable message.
include_guard(GLOBAL)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$"
   OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    message(FATAL_ERROR
        "Unsupported C++ compiler '${CMAKE_CXX_COMPILER_ID}' "
        "(frontend '${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}'). Use GCC or Clang with a "
        "GNU-style command line.")
endif()
