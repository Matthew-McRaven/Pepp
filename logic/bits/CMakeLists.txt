cmake_minimum_required(VERSION 3.10)

make_library(bits bits)
target_link_libraries(bits PUBLIC "Qt6::Core")
target_include_directories(bits PUBLIC ${Boost_INCLUDE_DIR})

if(NOT IOS)
  make_qtest(test-bits-copy test/copy.test.cpp "bits;Qt6::Core;Qt6::Test")
  set_target_properties(test-bits-copy PROPERTIES FOLDER "qtc_runnable")
endif()
