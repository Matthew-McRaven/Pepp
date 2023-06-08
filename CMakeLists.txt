cmake_minimum_required(VERSION 3.23)

project(Pepp VERSION 0.6.0 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# We always want universal builds, so do not set on a per-target basis
SET(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "" FORCE)
add_subdirectory(3rd-party)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(Qt6 6.4 COMPONENTS Quick Core Gui Qml Test Widgets REQUIRED)

# Emscripten build forget to set boost_INCLUDE_DIR correctly
set(Boost_INCLUDE_DIR "$ENV{BOOST_ROOT}")
find_package(Boost 1.76.0 REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
enable_testing()
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
include(config/cmake/create_targets.cmake)
add_subdirectory(logic)
add_subdirectory(bin)

# Derived from: https://vicrucann.github.io/tutorials/quick-cmake-doxygen/
# first we can indicate the documentation build as an option and set it to ON by default
option(BUILD_DOC "Build documentation" ON)

# check if Doxygen is installed
find_package(Doxygen)
if (DOXYGEN_FOUND)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    # note the option ALL which allows to build the docs together with the application
    #add_custom_target( doc_doxygen ALL
    #    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    #    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    #    COMMENT "Generating API documentation with Doxygen"
    #    VERBATIM )
else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)
