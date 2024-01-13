cmake_minimum_required(VERSION 3.23)

# set the project name
project(catch VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(catch INTERFACE catch.hpp)
target_sources(catch INTERFACE ${CMAKE_CURRENT_LIST_DIR}/catch.cpp)
target_compile_definitions(catch INTERFACE CATCH_AMALGAMATED_CUSTOM_MAIN)
target_include_directories(catch INTERFACE .)
