###########################################################
## CMAKE SETUP
###########################################################

cmake_minimum_required(VERSION 3.2)
project(lru-cache)

########################################
# C++ VERSIONING
########################################

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###########################################################
## DEPENDENCIES
###########################################################

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_SOURCE_DIR}/cmake/Modules/"
)

###########################################################
## INCLUDES
###########################################################

# Need this top-level include for "tests/"
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

###########################################################
## EXAMPLES
###########################################################

add_subdirectory(examples)

########################################
# TESTS
########################################

add_subdirectory(tests)
