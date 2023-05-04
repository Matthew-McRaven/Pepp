if(NOT IOS)
  make_qtest(test-isa3-pep10-adda instr/adda.test.cpp "targets;Qt6::Core;${Boost_LIBRARIES};Qt6::Test")
  set_target_properties(test-isa3-pep10-adda  PROPERTIES FOLDER "qtc_runnable")
endif()
