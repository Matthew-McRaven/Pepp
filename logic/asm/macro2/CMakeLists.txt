cmake_minimum_required(VERSION 3.10)

include(../../../config/cmake/create_targets.cmake)

enable_testing()
make_library(macro macro)
target_include_directories(macro PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(macro PRIVATE "Qt6::Core;${Boost_LIBRARIES}")

if(NOT IOS)
  make_qtest(test-macro-registry test/registry.test.cpp "macro;Qt6::Core;${Boost_LIBRARIES};Qt6::Test")
  make_qtest(test-macro-parse test/parse.test.cpp "macro;Qt6::Core;${Boost_LIBRARIES};Qt6::Test")
endif()
