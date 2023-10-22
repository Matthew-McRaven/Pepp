cmake_minimum_required(VERSION 3.23)

# set the project name
project(catch VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_library(catch STATIC catch.cpp)
target_include_directories(catch PUBLIC .)
