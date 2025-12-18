include(ExternalProject)

add_subdirectory(catch)
add_subdirectory(elfio)
add_subdirectory(cli)
# Disable unit tests for antlr, since it produces many CMake warnings. We don't
# use VCPKG, since it ships only x86 binaries for Mac, and we target ARM plus
# x86, which means we need to build from the sources.
set(ANTLR_BUILD_CPP_TESTS CACHE INTERNAL FALSE FORCE)
# On Windows, we ship with MSVC redistributable. Mismatch on static CRT linkage
# causes compile errors on Windows.
set(WITH_STATIC_CRT CACHE INTERNAL FALSE FORCE)
add_subdirectory(antlr4)

# Can be removed when https://github.com/eyalz800/zpp_bits/pull/130 is merged.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(zpp_bits INTERFACE zpp_bits/zpp_bits.h)
target_include_directories(zpp_bits INTERFACE zpp_bits)

add_subdirectory(lru)
add_subdirectory(scintilla)
set(FMT_INSTALL
    OFF
    CACHE BOOL "" FORCE)
add_subdirectory(fmt)
add_subdirectory(spdlog)

set(KDDockWidgets_FRONTENDS
    "qtquick"
    CACHE STRING "" FORCE)
set(KDDW_FRONTEND_QTQUICK
    ON
    CACHE BOOL "" FORCE)
set(KDDockWidgets_STATIC
    TRUE
    CACHE BOOL "" FORCE)
set(KDDockWidgets_QML_MODULE
    TRUE
    CACHE BOOL "" FORCE)
set(KDAB_INSTALL
    OFF
    CACHE BOOL "" FORCE)
add_subdirectory(KDDockWidgets/)

# CXXGraph has some annoying packaging "stuff" for being a header-only library
# (e.g., a CPM.cmake requirement). So, I grabbed the most recent release, and
# disable the ability to compile/update/install it. Using a release because the
# release sets the correct values for the config header version.
ExternalProject_Add(
  cxxgraph_helper
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/cxxgraph
  CONFIGURE_COMMAND "" # no configure step (header-only)
  BUILD_COMMAND "" # no build step
  INSTALL_COMMAND "" # no install step
  UPDATE_COMMAND "" # disable VCS updates
)
# External project is UTILITY, which has issues with connecting to my existing
# targets. Create a dummy target which propogates include directories and
# ensures that the code is cloned.
add_library(cxxgraph INTERFACE)
target_include_directories(cxxgraph
                           INTERFACE ${CMAKE_CURRENT_LIST_DIR}/cxxgraph/include)
add_dependencies(cxxgraph cxxgraph_helper)

# Inject --bind flag into KDDockWidgets targets on WASM builds
if(EMSCRIPTEN)
  get_directory_property(
    submodule_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/KDDockWidgets
                                BUILDSYSTEM_TARGETS)
  foreach(tgt IN LISTS submodule_targets)
    get_target_property(existing_flags ${tgt} LINK_FLAGS)
    if(NOT existing_flags)
      set(existing_flags "")
    endif()
    set_target_properties(${tgt} PROPERTIES LINK_FLAGS
                                            "${existing_flags} --bind")
  endforeach()
endif()

file(
  DOWNLOAD
  https://github.com/githubnext/monaspace/releases/download/v1.301/monaspace-static-v1.301.zip
  "${CMAKE_CURRENT_BINARY_DIR}/monaspace.zip"
  EXPECTED_HASH
    "SHA256=e201697c8ef3cfd4559fbfb4b5f60bf7425113729bca28e33b0a9112db1ca579")
file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_BINARY_DIR}/monaspace.zip
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/monaspace)
set(monaspace_SOURCE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/monaspace"
    PARENT_SCOPE)

include(Flat.cmake)

set(RISCV_64I
    OFF
    CACHE BOOL "" FORCE)
set(RISCV_BINARY_TRANSLATION
    OFF
    CACHE BOOL "" FORCE)

# libriscv does not create a .lib on Windows... so we'll just compile it as a static lib.
# For all other platofrms, respect the requested library build type.
if (MSVC)
  cmake_policy(PUSH)
  cmake_policy(SET CMP0077 NEW) # cache values override option() defaults in subdirs
  set(_saved_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")
  set(BUILD_SHARED_LIBS OFF)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static deps" FORCE)
  add_subdirectory(libriscv)
  set_property(TARGET riscv PROPERTY POSITION_INDEPENDENT_CODE ON)
  set_property(TARGET riscv PROPERTY MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)
  # C4661 deals with explicit template instantiation C4005 silences macro redefinition warnings.
  target_compile_options(riscv PRIVATE /wd4661 /wd4005)
  set(BUILD_SHARED_LIBS "${_saved_BUILD_SHARED_LIBS}")
  set(BUILD_SHARED_LIBS "${_saved_BUILD_SHARED_LIBS}" CACHE BOOL "" FORCE)
  cmake_policy(POP)
  unset(_saved_BUILD_SHARED_LIBS)
else()
  add_subdirectory(libriscv)
endif()
