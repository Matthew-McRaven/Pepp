cmake_minimum_required(VERSION 3.10)

include(../../../config/cmake/create_targets.cmake)

enable_testing()
make_library(symbol symbol)
target_include_directories(symbol PRIVATE fmt)
target_link_libraries(symbol PRIVATE "fmt;Qt6::Core")

if(NOT IOS)
make_test(symbol-test test "symbol;Qt6::Core")
endif()
