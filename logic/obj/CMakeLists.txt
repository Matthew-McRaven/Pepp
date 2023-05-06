cmake_minimum_required(VERSION 3.10)

make_library(obj obj)
target_link_libraries(obj PUBLIC "Qt6::Core;bits;elfio;isa")
target_include_directories(obj PUBLIC ${Boost_INCLUDE_DIR})

if(NOT IOS)
  #make_qtest(test-obj-roundtrip test/roundtrip.test.cpp "pas;obj;Qt6::Core;Qt6::Test;builtins")
  #set_target_properties(test-obj-roundtrip  PROPERTIES FOLDER "qtc_runnable")
endif()
