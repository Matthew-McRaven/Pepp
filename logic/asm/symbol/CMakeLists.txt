cmake_minimum_required(VERSION 3.10)

make_library(symbol symbol)
target_link_libraries(symbol PRIVATE "Qt6::Core")
target_compile_definitions(symbol PRIVATE SYMBOL_LIBRARY)

if(NOT IOS)
  make_qtest(test-symbol-ntable test/n_table.cpp "symbol;Qt6::Core;Qt6::Test")
  make_qtest(test-symbol-1table test/one_table.cpp "symbol;Qt6::Core;Qt6::Test")
  make_qtest(test-symbol-2table test/two_table.cpp "symbol;Qt6::Core;Qt6::Test")
  make_qtest(test-symbol-multitree test/multitree.cpp "symbol;Qt6::Core;Qt6::Test")
  make_qtest(test-symbol-values test/values.cpp "symbol;Qt6::Core;Qt6::Test")
endif()
