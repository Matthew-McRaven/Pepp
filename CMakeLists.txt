cmake_minimum_required(VERSION 3.23)

# Require explicit opt-out of vcpkg's boost.
if( (DEFINED USE_SYSTEM_BOOST) OR (DEFINED ENV{USE_SYSTEM_BOOST}) )
    message("Using system boost")
else()
    list(APPEND VCPKG_MANIFEST_FEATURES "boost")
    message("Using vcpkg boost")
endif()

set(Boost_USE_STATIC_LIBS   ON)
# Allow overriding boost root if you want to point to your own boost. Must also define Boost_ROOT.
set(Boost_ROOT ${_VCPKG_INSTALLED_DIR} CACHE PATH "Override boost root")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# We always want universal builds, so do not set on a per-target basis
SET(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "" FORCE)



# Emscripten build forget to set boost_INCLUDE_DIR correctly
set(Boost_USE_STATIC_LIBS   ON)
set(Boost_ROOT ${_VCPKG_INSTALLED_DIR} CACHE PATH "Override boost root")


# Derived from: https://vicrucann.github.io/tutorials/quick-cmake-doxygen/
# first we can indicate the documentation build as an option and set it to ON by default
# Can be BUILD, DOC, VERSION
set(ONLY "BUILD" CACHE STRING "Only build code/documentation/version" )


# Only set language one actual builds
if(ONLY STREQUAL "BUILD")
  set(LANGUAGES "CXX")
endif()

project(Pepp VERSION 0.6.0 LANGUAGES ${LANGUAGES})
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# check if Doxygen is installed, and if we have python for Sphinx
find_package(Doxygen)
find_program(SPHINX_EXECUTABLE
             NAMES sphinx-build
             DOC "Path to sphinx-build executable")

macro(generate_docs)
  set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
  # set input and output files
  set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in)
  set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  # request to configure the file
  configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
  message("Doxygen build started")
  message(${DOXYGEN_OUTPUT_DIR})
  # note the option ALL which allows to build the docs together with the application
  add_custom_target(doc_doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM )
  add_custom_target(docs_sphinx ALL
    COMMAND ${SPHINX_EXECUTABLE} -b html
      -Dbreathe_projects.all=${DOXYGEN_OUTPUT_DIR}/xml
      -Dversion=${CMAKE_PROJECT_VERSION}
      ${CMAKE_CURRENT_LIST_DIR}/docs ${CMAKE_CURRENT_BINARY_DIR}/docs
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Generating documentation with Sphinx"
    VERBATIM)
  add_dependencies(docs_sphinx doc_doxygen)
endmacro()

if(ONLY STREQUAL "BUILD")
  message("Performing build")
  find_package(Boost 1.76.0 REQUIRED)
  find_package(Qt6 6.4 COMPONENTS Quick Core Gui Qml Test Widgets REQUIRED)
  add_subdirectory(3rd-party)
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  enable_testing()
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  include(config/cmake/create_targets.cmake)
  include(config/cmake/enumerate_targets.cmake)
  add_subdirectory(logic)
  add_subdirectory(bin)

  # For emscripten builds, enumerate the file from projects that we would like to upload to our docs website.
  if(EMSCRIPTEN)
    # Enumerate all source directories which may have a sample demo program we want to display
    enumerate_tagged_targets(TaggedTargets bin "EMBED_GH_PAGES")
    # We need python to do some string manipulation
    find_package(Python REQUIRED COMPONENTS Interpreter)

    # Make output dir, since CMake won't do this by itself
    add_custom_target(make-ghpages COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/GH_PAGES")
    # Copy the names of all of the wasm outputs to a CSV file
    set(outfile "${CMAKE_CURRENT_BINARY_DIR}/GH_PAGES.csv")
    add_custom_target(
      gen-wasm-csv ALL
      COMMAND ${Python_EXECUTABLE}  ${CMAKE_CURRENT_LIST_DIR}/scripts/merge-wasm-csv.py "${outfile}" "${CMAKE_CURRENT_BINARY_DIR}/GH_PAGES"
    )

    # Enumerate tagged targets, and record their filenames
    # Each target needs its own sub-target, otherwise they will conflict on output files
    # There's probably some generator expression cleverness I could use, but it escapes me now.
    foreach(tgt IN LISTS TaggedTargets)
      add_custom_target(
          ${tgt}-wasminfo
          DEPENDS make-ghpages
          COMMAND ${CMAKE_COMMAND} -E echo \"$<TARGET_FILE:${tgt}>\" > \"${CMAKE_CURRENT_BINARY_DIR}/GH_PAGES/${tgt}.txt\"
      )
      add_dependencies(gen-wasm-csv ${tgt}-wasminfo)
    endforeach()
  endif()

  if(DOXYGEN_FOUND AND SPHINX_EXECUTABLE_FOUND)
    generate_docs()
  endif()
elseif(ONLY STREQUAL "DOC")
  message("Performing docs")
  if(DOXYGEN_FOUND AND SPHINX_EXECUTABLE_FOUND)
    generate_docs()
  else()
    message("Doxygen and Python need to be installed to generate the documentation")
  endif()
elseif(ONLY STREQUAL "VERSION")
  # Write version number out to file, so we can compare against git.
  file(WRITE version ${PROJECT_VERSION})
endif()

