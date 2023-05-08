cmake_minimum_required(VERSION 3.10)

make_library(targets .)
target_include_directories(targets PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(targets PUBLIC "Qt6::Core;${Boost_LIBRARIES};bits;sim;isa;elfio;obj")
target_compile_definitions(targets PRIVATE TARGETS_LIBRARY)

if(NOT IOS)
  make_qtest(test-targets-isa3-pep10-bootstrap1 test/isa3-pep10/bootstrap1.test.cpp "targets;Qt6::Core;${Boost_LIBRARIES};Qt6::Test")
  set_target_properties(test-targets-isa3-pep10-bootstrap1  PROPERTIES FOLDER "qtc_runnable")
  add_subdirectory(test/isa3-pep10)
endif()
