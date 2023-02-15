cmake_minimum_required(VERSION 3.10)

include(../../../config/cmake/create_targets.cmake)

enable_testing()
make_library(macro macro)
target_include_directories(macro PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(macro PRIVATE "Qt6::Core;${Boost_LIBRARIES}")

if(NOT IOS)
make_test(macro-test test "macro;Qt6::Core;${Boost_LIBRARIES}")
endif()
