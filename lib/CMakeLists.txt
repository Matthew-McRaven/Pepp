cmake_minimum_required(VERSION 3.10)
inject_cxx_standard()
inject_clang_tidy()
inject_dwarf_debug()
inject_code_coverage()

# Extract git info
include(help/about/git.cmake)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_get_exact_tag(GIT_TAG)
if(GIT_TAG MATCHES ".*NOTFOUND")
  set(GIT_TAG "unknown")
endif()
# Let program know if it was built from a clean dir. App can use that to render
# about info.
git_local_changes(GIT_CLEAN_RAW)
if(GIT_CLEAN_RAW MATCHES "CLEAN")
  set(GIT_CLEAN "1")
else()
  set(GIT_CLEAN "0")
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/help/about/version_gen.cpp.in"
               "${CMAKE_CURRENT_BINARY_DIR}/version_gen.cpp" @ONLY)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
# GLOB_RECURSE uses abs paths, QML_FILES expects relative paths. This blob
# converts from abs to rel.
file(GLOB_RECURSE abs_qml_sources CONFIGURE_DEPENDS "*.qml")
set(rel_qml_sources, "")
foreach(f_abs IN LISTS abs_qml_sources)
  file(RELATIVE_PATH f_rel "${CMAKE_CURRENT_LIST_DIR}" ${f_abs})
  list(APPEND rel_qml_sources "${f_rel}")
endforeach()
qt_add_qml_module(
  pepp-lib
  URI
  "edu.pepp"
  VERSION
  1.0
  CLASS_NAME
  PeppLibPlugin
  SOURCES
  ${sources}
  "${CMAKE_CURRENT_BINARY_DIR}/version_gen.cpp"
  "${CMAKE_SOURCE_DIR}/resource_license.qrc"
  QML_FILES
  ${rel_qml_sources}
  DEPENDENCIES
  QtQuick
  OUTPUT_DIRECTORY
  "${CMAKE_BINARY_DIR}/edu/pepp")
target_compile_definitions(pepp-lib PRIVATE "PEPP_LIBRARY")
target_link_libraries(
  pepp-lib
  PUBLIC Qt6::Core
         Qt6::Gui
         Qt6::Quick
         Qt6::Sql
         antlr4_shared
         elfio
         lru-cache
         scintilla_qt
         zpp_bits
         fmt
         spdlog
         cxxgraph
         pepp-changelog)
# Will fail to find hpp files without this, see
# https://bugreports.qt.io/browse/QTBUG-101146.
get_target_property(ANTLR4_INCLUDE_DIR antlr4_shared INCLUDE_DIRECTORIES)
target_include_directories(pepp-lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
                                           ${ANTLR4_INCLUDE_DIR})
target_include_directories(
  pepp-lib
  PRIVATE enums
          cpu
          helpers
          help/about
          help/builtins
          enums/isa
          toolchain/link
          toolchain/macro
          memory
          memory/hexdump
          memory/stack
          preferences
          project
          sim
          sim/debug
          targets
          text
          utils
          text/editor
          menu
          settings
          top)

# Add additional JS support files
if(EMSCRIPTEN)
  target_link_libraries(pepp-lib PRIVATE idbfs.js)
  target_link_options(pepp-lib PRIVATE "SHELL:-s FORCE_FILESYSTEM=1 ")
endif()

# Include additional fonts
file(GLOB_RECURSE fonts CONFIGURE_DEPENDS "${PROJECT_DATA_DIR}/fonts/*")
qt_add_resources(
  pepp-lib
  "fonts"
  PREFIX
  "/fonts"
  BASE
  "${PROJECT_DATA_DIR}/fonts"
  FILES
  ${fonts})

file(GLOB_RECURSE fonts-dl CONFIGURE_DEPENDS "${monaspace_SOURCE_DIR}/*")
list(FILTER fonts-dl EXCLUDE REGEX ".*(Extra|Light|Medium|Wide|Semi).*")
qt_add_resources(
  pepp-lib
  "fonts-dl"
  PREFIX
  "/fonts/mono"
  BASE
  "${monaspace_SOURCE_DIR}/"
  # This will cause issues in IOS, bug we can cross that bridge later. See:
  # https://doc.qt.io/qt-6/qt-add-resources.html#arguments-of-the-target-based-variant
  BIG_RESOURCES
  FILES
  ${fonts-dl})

# Include default themes without having to hard-code.
file(GLOB_RECURSE themes CONFIGURE_DEPENDS "${PROJECT_DATA_DIR}/themes/*")
qt_add_resources(
  pepp-lib
  "themes"
  PREFIX
  "/themes"
  BASE
  "${PROJECT_DATA_DIR}/themes"
  FILES
  ${themes})

# Include dependency licenses without having to hard-code.
file(GLOB_RECURSE dependency_licenses CONFIGURE_DEPENDS
     "${PROJECT_DATA_DIR}/about/dep/*")
set_source_files_properties(${CMAKE_SOURCE_DIR}/NOTICE
                            PROPERTIES QT_RESOURCE_ALIAS LICENSE_NOTICE)
set_source_files_properties(${CMAKE_SOURCE_DIR}/LICENSE
                            PROPERTIES QT_RESOURCE_ALIAS LICENSE_FULL)
qt_add_resources(
  pepp-lib
  "about"
  PREFIX
  "/about"
  BASE
  "${PROJECT_DATA_DIR}/about"
  FILES
  ${PROJECT_DATA_DIR}/about/contributors.csv
  ${PROJECT_DATA_DIR}/about/maintainers.csv
  ${PROJECT_DATA_DIR}/about/repo.url
  ${PROJECT_DATA_DIR}/about/dependencies.csv
  ${dependency_licenses}
  ${CMAKE_SOURCE_DIR}/NOTICE
  ${CMAKE_SOURCE_DIR}/LICENSE)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(pepp-lib PUBLIC "PEPP_LIB_SHARED_BUILD=1")
else()
  target_compile_definitions(pepp-lib PUBLIC "PEPP_LIB_STATIC_BUILD=1")
endif()

# Add book resources
file(GLOB_RECURSE cs-res-abs CONFIGURE_DEPENDS
     "${PROJECT_DATA_DIR}/books/cs4e/*" "${PROJECT_DATA_DIR}/books/cs5e/*"
     "${PROJECT_DATA_DIR}/books/cs6e/*")
# Remove generated / debug files from our book resources
list(FILTER cs-res-abs EXCLUDE REGEX "\.DS_Store")
list(FILTER cs-res-abs EXCLUDE REGEX "\.idea")
list(FILTER cs-res-abs EXCLUDE REGEX "__pycache__")
list(FILTER cs-res-abs EXCLUDE REGEX "\..*_cache")
list(FILTER cs-res-abs EXCLUDE REGEX "\.pdf")
set(ce-res, "")
foreach(f-abs IN LISTS cs-res-abs)
  file(RELATIVE_PATH f-rel "${CMAKE_CURRENT_LIST_DIR}" ${f-abs})
  list(APPEND cs-res "${f-rel}")
endforeach()
qt_add_resources(
  pepp-lib
  "builtins"
  PREFIX
  "/"
  BASE
  "${PROJECT_DATA_DIR}/"
  FILES
  ${cs-res})
# Add help resources
file(GLOB_RECURSE help-res-abs CONFIGURE_DEPENDS "${PROJECT_DATA_DIR}/help/*")
set(help-res, "")
foreach(f-abs IN LISTS help-res-abs)
  file(RELATIVE_PATH f-rel "${CMAKE_CURRENT_LIST_DIR}" ${f-abs})
  list(APPEND help-res "${f-rel}")
endforeach()
qt_add_resources(
  pepp-lib
  "help"
  PREFIX
  "/help"
  BASE
  "${PROJECT_DATA_DIR}/help"
  FILES
  ${help-res})
