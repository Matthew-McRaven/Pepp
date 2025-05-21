###########################################################
## CMAKE SETUP
###########################################################

cmake_minimum_required(VERSION 3.20)
project(lru-cache-proj)

########################################
# C++ VERSIONING
########################################

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###########################################################
## INCLUDES
###########################################################
find_package(Qt6 6.4 COMPONENTS REQUIRED Core)
add_library(lru-cache INTERFACE)
target_include_directories(lru-cache INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(NOT IOS)
    add_subdirectory(examples)
    add_subdirectory(tests)
endif()
