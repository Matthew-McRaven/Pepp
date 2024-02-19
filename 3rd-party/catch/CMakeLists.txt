# Adapted from from Catch2 Commit b817497528877b14d6102869a6df378d029b343b
# See: https://raw.githubusercontent.com/catchorg/Catch2/b817497528877b14d6102869a6df378d029b343b/extras/CatchShardTests.cmake
cmake_minimum_required(VERSION 3.23)

# set the project name
project(catch VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Link against this target to be able to use catch features.
add_library(catch INTERFACE catch.hpp)
target_sources(catch INTERFACE ${CMAKE_CURRENT_LIST_DIR}/catch.cpp)
target_compile_definitions(catch INTERFACE CATCH_AMALGAMATED_CUSTOM_MAIN)
target_include_directories(catch INTERFACE .)

# Link against this target to get catch plus a default main.
add_library(catch-main INTERFACE catch_main.cpp)
target_sources(catch-main INTERFACE ${CMAKE_CURRENT_LIST_DIR}/catch_main.cpp)
target_link_libraries(catch-main INTERFACE catch)

#########################
# CatchShardTests.cmake #
#########################

#              Copyright Catch2 Authors
# Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE.txt or copy at
#        https://www.boost.org/LICENSE_1_0.txt)

# SPDX-License-Identifier: BSL-1.0

# TARGET is a normal executable cmake target
# SUFFIX is a value that, if appended to the TARGET's file name, gives you the correct target file.
# This was necessary to get around issues with symlinks not being real targets.
# Supported optional args:
#  * SHARD_COUNT - number of shards to split target's tests into
#  * REPORTER    - reporter spec to use for tests
#  * TEST_SPEC   - test spec used for filtering tests
function(catch_add_sharded_tests TARGET)
  if (${CMAKE_VERSION} VERSION_LESS "3.10.0")
    message(FATAL_ERROR "add_sharded_catch_tests only supports CMake versions 3.10.0 and up")
  endif()

  cmake_parse_arguments(
    ""
    ""
    "SHARD_COUNT;REPORTER;TEST_SPEC;SUFFIX;ADDTL_ARGS"
    ""
    ${ARGN}
  )

  if (NOT DEFINED _SHARD_COUNT)
    set(_SHARD_COUNT 2)
  endif()

  # Generate a unique name based on the extra arguments
  string(SHA1 args_hash "${_TEST_SPEC} ${_EXTRA_ARGS} ${_REPORTER} ${_OUTPUT_DIR} ${_OUTPUT_PREFIX} ${_OUTPUT_SUFFIX} ${_SHARD_COUNT}")
  string(SUBSTRING ${args_hash} 0 7 args_hash)

  set(ctest_include_file "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}-sharded-tests-include.cmake")
  set(ctest_tests_file "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}-sharded-tests-impl.cmake")

  file(WRITE "${ctest_include_file}"
    "if(EXISTS \"${ctest_tests_file}\")\n"
    "  include(\"${ctest_tests_file}\")\n"
    "else()\n"
    "  add_test(${TARGET}_NOT_BUILT ${TARGET}_NOT_BUILT)\n"
    "endif()\n"
  )

  set_property(DIRECTORY
    APPEND PROPERTY TEST_INCLUDE_FILES "${ctest_include_file}"
  )

  set(shard_impl_script_file "${PROJECT_SOURCE_DIR}/3rd-party/catch/ShardImpl.cmake")

  # They told me you could join strings in genexprs.
  # The line below treats strings as lists and concats them with an empty seperator.
  # This looks an awful lot like string concatentation.
  # If suffix is empy, we need to strip it to prevent unwanted spaces from appearing.
  string(STRIP "${_SUFFIX}" _SUFFIX)
  message("CTest file: ${ctest_tests_file}")
  add_custom_command(
    TARGET ${TARGET} POST_BUILD
    BYPRODUCTS "${ctest_tests_file}"
    COMMAND "${CMAKE_COMMAND}"
            -D "TARGET_NAME=${TARGET}"
            # Use join<append> to concatenate strings
            # Get the directory in which the test executable is installed, add a slash so we are inside it.
            # Get the target basename (without suffix or extensions). Append both the suffix and extension.
            # Trailing ,> joins the list with an empty separator -- which is string concatenation.
            -D "TEST_BINARY=$<LIST:JOIN,$<LIST:APPEND,$<TARGET_FILE_DIR:${TARGET}>/,$<TARGET_FILE_BASE_NAME:${TARGET}>,${_SUFFIX},$<PATH:GET_EXTENSION,$<TARGET_FILE:${TARGET}>>>,>"
            -D "CTEST_FILE=${ctest_tests_file}"
            -D "SHARD_COUNT=${_SHARD_COUNT}"
            -D "ADDTL_ARGS=${_ADDTL_ARGS}"
            -P "${shard_impl_script_file}"
    VERBATIM
  )
endfunction()
