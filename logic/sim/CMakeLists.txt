cmake_minimum_required(VERSION 3.10)

enable_testing()
make_library(sim .)
target_include_directories(sim PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(sim PUBLIC "Qt6::Core;${Boost_LIBRARIES}")
target_compile_definitions(sim PRIVATE SIM_LIBRARY)

if(NOT IOS)
  make_qtest(test-sim-device-dense test/device/dense.test.cpp "sim;Qt6::Core;${Boost_LIBRARIES};Qt6::Test")
  set_target_properties(test-sim-device-dense  PROPERTIES FOLDER "qtc_runnable")
endif()
