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
