include(ExternalProject)

include(Json.cmake)
include(zpp_bits.cmake)
include(Flat.cmake)
include(cnl.cmake)
add_subdirectory(catch)
set(FMT_INSTALL
    OFF
    CACHE BOOL "" FORCE)
add_subdirectory(fmt)

add_subdirectory(elfio)
add_subdirectory(cli)
add_subdirectory(lru)
add_subdirectory(spdlog)

# Do not build tests while building python bindings. Huge slow down. If you
# remove EMSCRIPTEN from this list of conditions, update /test/CMakeLists.txt
if(EMSCRIPTEN OR NOT PEPP_BUILD_GUI)
  set(MOCKTURTLE_BUILD_TESTS
      OFF
      CACHE BOOL "" FORCE)
else(PEPP_BUILD_GUI)
  # Avoid building mockturtle until we start simulating circuits.
  set(MOCKTURTLE_BUILD_TESTS
      OFF
      CACHE BOOL "" FORCE)
endif()
add_subdirectory(mockturtle)

# Can be removed when https://github.com/eyalz800/zpp_bits/pull/130 is merged.
if(Python_VERSION VERSION_GREATER_EQUAL "3.12"
   AND NOT EMSCRIPTEN
   AND PEPP_BUILD_PYTHON)
  add_subdirectory(nanobind)
endif()
if(PEPP_BUILD_GUI)
  # Disable unit tests for antlr, since it produces many CMake warnings. We
  # don't use VCPKG, since it ships only x86 binaries for Mac, and we target ARM
  # plus x86, which means we need to build from the sources.
  set(ANTLR_BUILD_CPP_TESTS CACHE INTERNAL FALSE FORCE)
  # On Windows, we ship with MSVC redistributable. Mismatch on static CRT
  # linkage causes compile errors on Windows.
  set(WITH_STATIC_CRT CACHE INTERNAL FALSE FORCE)
  # Build only the antlr4 runtime flavor we actually link. Default builds both,
  # which wastes compile time.
  if(BUILD_SHARED_LIBS)
    set(ANTLR_BUILD_SHARED
        ON
        CACHE BOOL "" FORCE)
    set(ANTLR_BUILD_STATIC
        OFF
        CACHE BOOL "" FORCE)
    set(PEPP_ANTLR4_TARGET antlr4_shared)
  else()
    set(ANTLR_BUILD_SHARED
        OFF
        CACHE BOOL "" FORCE)
    set(ANTLR_BUILD_STATIC
        ON
        CACHE BOOL "" FORCE)
    set(PEPP_ANTLR4_TARGET antlr4_static)
  endif()
  add_subdirectory(antlr4)
  # Consumers link ${PEPP_ANTLR4_TARGET} rather than naming a flavor.
  set(PEPP_ANTLR4_TARGET
      ${PEPP_ANTLR4_TARGET}
      PARENT_SCOPE)

  add_subdirectory(scintilla)

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

  if(PEPP_PRECOMPILE_HEADERS)

    pepp_precompile_headers(scintilla_qt PRIVATE <QtCore>)
    pepp_precompile_headers(lexilla PRIVATE <QtCore>)
    pepp_precompile_headers(kddockwidgets PRIVATE <QtCore>)
    # The unit-test targets only inherited catch.hpp via catch's INTERFACE PCH.
    foreach(_t IN ITEMS lexilla_unit scintilla_unit_qt)
      if(TARGET ${_t})
        pepp_precompile_headers(${_t} PRIVATE <QtCore>)
      endif()
    endforeach()
    pepp_precompile_headers(
      ${PEPP_ANTLR4_TARGET} PRIVATE
      "${CMAKE_CURRENT_LIST_DIR}/antlr4/runtime/src/antlr4-common.h"
      "${CMAKE_CURRENT_LIST_DIR}/antlr4/runtime/src/Exceptions.h")
  endif()
endif()

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
