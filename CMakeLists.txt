cmake_minimum_required(VERSION 3.24)

# Needed so GENERATED is propogated See:
# https://cmake.org/cmake/help/latest/policy/CMP0118.html Discussion here:
# https://discourse.cmake.org/t/bug-with-generated-file-and-target-sources-private/5262/11
cmake_policy(SET CMP0118 NEW)

include(FetchContent)

# Needed to generate documentation, changelogs
find_package(Python 3.12 COMPONENTS Interpreter)
option(BUILD_SHARED_LIBS
       "Build using shared libraries. May be ignored on some platforms" ON)
option(PEPP_BUILD_PYTHON "Build only Python bindings" OFF)
option(PEPP_BUILD_GUI "Build GUI applications" ON)
# Allow normalized paths (e.g., "toolchain/pas" ) without nesting that directory
# folder inside a nested "src" hierarchy. May cause linking issues if you do not
# pay attention.
include_directories(${CMAKE_CURRENT_LIST_DIR}/lib)

option(SANITIZE "Enable sanitizers" OFF)
option(CODECOV "Enable code coverage" OFF)
option(TIME_TRACE "Enable clang time trace" OFF)
if(SANITIZE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-fsanitize=undefined -fsanitize=address)
  add_link_options(-fsanitize=undefined -fsanitize=address)
endif()
if(TIME_TRACE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-ftime-trace)
endif()

# DO NOT AUTO-FORMAT. CI looks for a line with this format to extract the
# version.
# cmake-format: off
project(Pepp VERSION 0.15.0 LANGUAGES C CXX)
# cmake-format: on

include(config/cmake/inject_defs.cmake)
# Not respected by all generators, but incredibly useful when debugging builds.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Needed to avoid exporting every symbol in every DLL on Windows. Some corner
# cases still require we export symbols manually.
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
inject_clang_tidy()
inject_dwarf_debug()
inject_code_coverage(${CODECOV})

# Only overwrite the install prefix if it is the default value.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX
      "${PROJECT_BINARY_DIR}/install"
      CACHE PATH "Installation Directory" FORCE)
endif()

# 0 is TRACE, which enables all logging in every location. Only works if set as
# a global compile option and before 3rd-party. Default to 3, since I am tired
# of seeing all of my  info messages.
add_compile_options(-DSPDLOG_ACTIVE_LEVEL=3)

if(EMSCRIPTEN)
  add_compile_options(-O1 -gsource-map)
  add_link_options(-sALLOW_MEMORY_GROWTH=1 -sTOTAL_MEMORY=1GB
                   -sINITIAL_MEMORY=150MB -g2 -gsource-map)
  set(QT_WASM_INITIAL_MEMORY
      150MB
      CACHE INTERNAL "" FORCE)
  set(BUILD_SHARED_LIBS FALSE)
endif()

# Use to all output object files (applications, libraries, etc) in a single
# directory. Without this, Windows does a bad job of copying DLLs on change for
# the terminal application. However, it confuses iOS builds and must be disabled
# there.
if(NOT IOS)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output")
endif()
set(PROJECT_DATA_DIR "${CMAKE_CURRENT_LIST_DIR}/data")

if(PEPP_BUILD_GUI)
  set(QT_QML_GENERATE_QMLLS_INI ON)
  # We always want universal builds, so do not set on a per-target basis
  find_package(
    Qt6 6.9
    COMPONENTS
    REQUIRED
    Svg
    Quick
    Core
    Gui
    Qml
    Test
    Widgets
    QuickControls2
    Xml
    Sql
    OPTIONAL_COMPONENTS Concurrent)
  qt_policy(SET QTP0003 NEW) # Use BUILD_SHARED_LIBS as the default library type
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
endif()

# Do not enable QtConcurrent on WASM at this time.
if(TARGET Qt6::Concurrent AND NOT EMSCRIPTEN)
  add_compile_definitions(PEPP_HAS_QTCONCURRENT=1)
else()
  add_compile_definitions(PEPP_HAS_QTCONCURRENT=0)
endif()

if(Qt6_VERSION VERSION_GREATER_EQUAL 6.8)
  # Qt6.8/WASM stopped building without this flag being set.
  if(EMSCRIPTEN)
    add_link_options(--bind)
  endif()
  qt_policy(SET QTP0001 NEW)
  qt_policy(SET QTP0004 NEW)
endif()

add_subdirectory(3rd-party)

enable_testing()

include(config/compile_tests/TryBuildStdRanges.cmake)
include(config/cmake/FindRISCVToolchain.cmake)
add_subdirectory(core)
if(PEPP_BUILD_GUI)
  # Creates special test targets.
  include(config/cmake/create_targets.cmake)
  include(config/compile_tests/TryFindWebP.cmake)
  include(config/cmake/GenerateChangelog.cmake)
  add_subdirectory(lib)
  add_subdirectory(test)
  add_subdirectory(test2)
endif()
add_subdirectory(bin)
