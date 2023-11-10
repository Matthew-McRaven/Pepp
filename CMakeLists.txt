cmake_minimum_required(VERSION 3.23)

# Needed so GENERATED is propogated
# See: https://cmake.org/cmake/help/latest/policy/CMP0118.html
# Discussion here: https://discourse.cmake.org/t/bug-with-generated-file-and-target-sources-private/5262/11
# Needs CMake 3.20, hence line 1.
cmake_policy(SET CMP0118 NEW)

# My boost-version selection breaks when inside the if(BUILD) block
# Require explicit opt-out of vcpkg's boost.
if( (DEFINED USE_SYSTEM_BOOST) OR (DEFINED ENV{USE_SYSTEM_BOOST}) )
    message("Using system boost")
else()
    list(APPEND VCPKG_MANIFEST_FEATURES "boost")
    message("Using vcpkg boost")
endif()
# Emscripten build forget to set boost_INCLUDE_DIR correctly
set(Boost_USE_STATIC_LIBS   ON)
# Allow overriding boost root if you want to point to your own boost. Must also define Boost_ROOT.
set(Boost_ROOT ${_VCPKG_INSTALLED_DIR} CACHE PATH "Override boost root")


# Derived from: https://vicrucann.github.io/tutorials/quick-cmake-doxygen/
# Can be BUILD, DOC, VERSION.
# BUILD: compiles the project normally
# DOC: Only run doxygen+sphinx documentation build steps
# VERSION: output the project version to ${CMAKE_CURRENT_LIST_DIR}/version
set(ONLY "BUILD" CACHE STRING "Only build code/documentation/version" )
# APP: Build the help documentation for the windows/mac/linux builds.
# WEB: Build the help documentation for the web build, including web assembly files.
# DEV: Build the help with code information and architecture documents.
set(DOC_AUDIENCE "APP" CACHE STRING "Which version of the documentation to build" FORCE)
# Must disable QRC logic if emscripten, because a developer may forget to set this flag.
if(EMSCRIPTEN)
  set(DOC_AUDIENCE "WEB")
endif()
# If set, skip documentation generation, and generate a QRC directly from this directory.
set(DOC_PREBUILT_DIR CACHE DIRECTORY "Directory in which pre-built help documentation can be found" FORCE)

# Only set language one actual builds. Otherwise it may try to find buildtools
# on platforms without support for C++. (e.g., documentation builds).
if(ONLY STREQUAL "BUILD")
  set(LANGUAGES "CXX")
endif()
project(Pepp VERSION 0.6.0 LANGUAGES ${LANGUAGES})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include(config/cmake/enumerate_targets.cmake)

# Directory in which to store generated documentation.
set(SPHINX_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs")
# If the documentation fails to build, use this QRC instead to remove a corner case form
# the web engine.
set(PLACEHOLDER_HELP_QRC ${CMAKE_CURRENT_LIST_DIR}/config/default_help/placeholder_help.qrc)

function(generate_docs SPHINX_OUT_DIR)
  # Check if Doxygen is installed, and if we have Sphinx
  find_package(Doxygen)
  find_program(SPHINX_EXECUTABLE
               NAMES sphinx-build
               DOC "Path to sphinx-build executable")

  set(SPHINX_IN_DIR "${CMAKE_CURRENT_LIST_DIR}/docs")
  if(SPHINX_EXECUTABLE STREQUAL "SPHINX_EXECUTABLE-NOTFOUND")
    message("Sphinx not found, skipping doc build")
    return()
  endif()

  # Must perform minor configuration of doxygen before sphinx.
  # Otherwise, sphinx will always fail to find the doxygen xml files if they are not generated.
  # Breathe does not silently fail if the dir does not exist.
  if(DOXYGEN_FOUND AND (DOC_AUDIENCE STREQUAL "DEV"))
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
    set(DOXY_OPTS "-Dbreathe_projects.all=${DOXYGEN_OUTPUT_DIR}/xml")
  endif()

  # In application builds, don't include a sidebar,
  # as it wastes useful screen space.
  set(NO_SPHINX_SIDEBAR false)
  if(DOC_AUDIENCE STREQUAL "APP")
    set(NO_SPHINX_SIDEBAR true)
  endif()

  # Location in build directory in which to store generated HTML/JS/WASM
  # Will later be copied to the sphinx build directory, so that sphinx can find it.
  set(SPHINX_COPY_GENERATED_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/__dist)

  # Must be in "all", or docs_sphinx will never be run.
  add_custom_target(docs_sphinx ALL
    COMMAND ${SPHINX_EXECUTABLE} -b html
      ${DOXY_OPTS}
      -Dhtml_theme_options.nosidebar=${SPHINX_SIDEBAR}
      -q
      -t ${DOC_AUDIENCE}
      -Dversion=${CMAKE_PROJECT_VERSION}
      # Copy will silently ignore directories which do not exist.
      # May be some weirdness on developer machines switching between web and app builds.
      -DabsPairs=${SPHINX_COPY_GENERATED_SOURCE}/?_generated,${SPHINX_IN_DIR}/_generated?_generated
      ${SPHINX_IN_DIR} ${SPHINX_OUT_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Generating documentation with Sphinx"
    VERBATIM
  )

  # Doxygen xml generation is not fast, so avoid it unless explicitly requested.
  if(DOXYGEN_FOUND AND (DOC_AUDIENCE STREQUAL "DEV"))
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    # Insert dependency of sphinx on doxygen to ensure that
    # doxygen is run.
    add_custom_target(doc_doxygen
      COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM )
    add_dependencies(docs_sphinx doc_doxygen)
  elseif(DOC_AUDIENCE STREQUAL "WEB")
    # Upgrade python to a requirement, since we use many python scripts to perform
    # various file manipulations.
    find_package(Python REQUIRED COMPONENTS Interpreter)
    # Enumerate all source directories which may have a sample demo program we want to display
    enumerate_tagged_targets(TaggedTargets bin "EMBED_GH_PAGES")

    # TODO: copy configure_file to ${SPHINX_COPY_GENERATED_SOURCE}.
    # TODO: make python script which generates RST files for each demo program.
    # TODO: make python program which generates the index.rst file from a directory containing RSTs.
    # Enumerate targets request web help injection,
    # then copy over wasm/js files after those targets are build
    foreach(tgt IN LISTS TaggedTargets)
      # Needed to force generate-demo-index to be dependent on all the wasm htmls
      list(APPEND targets-copy-wasm ${tgt}-copy-wasm)
      # Copy JS and WASM files
      add_custom_target(
        ${tgt}-copy-wasm
        DEPENDS ${tgt}
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/copy-wasmlike.py \"$<TARGET_FILE:${tgt}>\" ${SPHINX_COPY_GENERATED_SOURCE}
      )
      # Create the HTML which embeds the WASM+js.
      configure_file(${SPHINX_IN_DIR}/_templates/webasm.html.in ${SPHINX_IN_DIR}/_generated/${tgt}.html @ONLY)
    endforeach()
    # Create and index.rst that has the right toctree for each sample program
    # Create a <page>.rst that embeds the WASM.
    add_custom_target(
      generate-demo-index
      DEPENDS ${targets-copy-wasm}
      COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/generate-wasm-rst.py "${SPHINX_IN_DIR}/_generated"
    )
    # Force execution by making it a dependency of an ALL target.
    add_dependencies(docs_sphinx generate-demo-index)
  endif()
endfunction()

# Find all targets in the project that need injection of web help.
function(inject_help_qrc WEB_HELP_QRC)
  enumerate_tagged_targets(QRCHelpTargets bin "NEEDS_HTML_HELP")
  enumerate_tagged_targets(QRCHelpTargets logic "NEEDS_HTML_HELP")
  add_library(webqrc-generated OBJECT ${WEB_HELP_QRC})
  foreach(tgt IN LISTS QRCHelpTargets)
    # Avoids a bug/behavior where CMake's GENERATED property won't make it across directories
    # GENERATED will only work within this directory, so create bogus target which links
    # to the real target. Use OBJECT library to avoid intermediate .a/.lib files.
    add_dependencies(${tgt} docs_qrc webqrc-generated)
  endforeach()
endfunction()

# Helper to run python over a directory and generate a qrc file
function(help_qrc_from_dir DIR)
  # Place the qrc in the root of the build riectory.
  # We *MUST* mark it with additional generated/rcc flags, or it will not
  # be handled properly across directories.
  set(WEB_HELP_QRC ${CMAKE_CURRENT_BINARY_DIR}/webhelp.qrc)
  set_source_files_properties(${WEB_HELP_QRC} PROPERTIES GENERATED ON)
  set_source_files_properties(${WEB_HELP_QRC} PROPERTIES AUTORCC ON)

  # BYPRODUCTS not always setting GENERATED correctly.
  add_custom_target(docs_qrc
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/generate-doc-qrc.py ${DIR} ${WEB_HELP_QRC}
    BYPRODUCTS ${WEB_HELP_QRC}
  )
  # Find all targets which need web help and inject it into those targets.
  inject_help_qrc(${WEB_HELP_QRC})
endfunction()

# Helper to let us find sphinx build directory

if(ONLY STREQUAL "BUILD")
  message("Performing build")

  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  # We always want universal builds, so do not set on a per-target basis
  SET(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "" FORCE)

  find_package(Boost 1.76.0 REQUIRED)
  find_package(Qt6 6.4 COMPONENTS Quick Core Gui Qml Test Widgets REQUIRED)
  add_subdirectory(3rd-party)
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  enable_testing()
  # Needed to avoid exporting every symbol in every DLL on Windows.
  # Some corner cases still require we export symbols manually.
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  include(config/cmake/create_targets.cmake)
  add_subdirectory(logic)
  add_subdirectory(bin)

  # Needed to genrate documentation
  find_package(Python COMPONENTS Interpreter)

  # Remove generated documentation files on each re-run,
  # to prevent files from leaking between configurations.
  file(REMOVE_RECURSE ${CMAKE_CURRENT_LIST_DIR}/docs/_generated)
  generate_docs(${SPHINX_OUT_DIR})

  # Don't attempt to build docs when given a prebuilt version.
  # Doc generation also requires python, so skip that step if missing python.
  string(COMPARE EQUAL "${DOC_PREBUILT_DIR}" "" EMPTY_PREBUILT_DIR)
  if(EMPTY_PREBUILT_DIR OR (NOT Python_FOUND))
  endif()

  # Don't allow building QRC whenever we build for the web.
  # Attempting to embed the wasm content in a QRC creates a compile-loop.
  # Additionally, compiling >100MB of text into a QRC takes a long time.
  # We can just re-direct to the book website.
  string(COMPARE EQUAL "${DOC_AUDIENCE}" "WEB" SKIP_QRC)

  if(SKIP_QRC)
    message("Not generating any help QRC")
  elseif(Python_FOUND AND (NOT EMPTY_PREBUILT_DIR))
    message("Using prebuilt help to generate help QRC")
    help_qrc_from_dir(${DOC_PREBUILT_DIR} AND NOT SKIP_QRC)
  elseif(Python_FOUND AND (TARGET docs_sphinx))
    message("Use generated help to generate help QRC")
    help_qrc_from_dir(${SPHINX_OUT_DIR})
  else()
    message("Using placeholder help QRC")
    inject_help_qrc(${PLACEHOLDER_HELP_QRC})
  endif()
elseif(ONLY STREQUAL "DOC")
    generate_docs(${SPHINX_OUT_DIR})
elseif(ONLY STREQUAL "VERSION")
  # Write version number out to file, so we can compare against git.
  file(WRITE version ${PROJECT_VERSION})
endif()

