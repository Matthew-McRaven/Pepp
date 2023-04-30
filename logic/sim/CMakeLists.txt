cmake_minimum_required(VERSION 3.10)

enable_testing()
make_library(sim .)
target_include_directories(sim PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(sim PUBLIC "Qt6::Core;${Boost_LIBRARIES}")
target_compile_definitions(sim PRIVATE DEVICE_LIBRARY)

if(NOT IOS)
  #make_qtest(test-macro-registry test/registry.test.cpp "macro;Qt6::Core;${Boost_LIBRARIES};Qt6::Test")
  #make_qtest(test-macro-parse test/parse.test.cpp "macro;Qt6::Core;${Boost_LIBRARIES};Qt6::Test")
endif()
