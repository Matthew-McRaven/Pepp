# Adapted from from Catch2 Commit b817497528877b14d6102869a6df378d029b343b
# See: https://raw.githubusercontent.com/catchorg/Catch2/b817497528877b14d6102869a6df378d029b343b/extras/CatchShardTests.cmake
cmake_minimum_required(VERSION 3.24)

# set the project name
project(catch VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
# Link against this target to be able to use catch features.
add_library(catch ${CMAKE_CURRENT_LIST_DIR}/catch.cpp)
set_target_properties(catch PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(catch PUBLIC CATCH_AMALGAMATED_CUSTOM_MAIN)
target_include_directories(catch PUBLIC .)

pepp_precompile_headers(catch PUBLIC "${CMAKE_CURRENT_LIST_DIR}/catch.hpp")

# Link against this target to get catch plus a default main.
add_library(catch-main INTERFACE catch_main.cpp)
target_sources(catch-main INTERFACE ${CMAKE_CURRENT_LIST_DIR}/catch_main.cpp)
target_link_libraries(catch-main INTERFACE catch)
