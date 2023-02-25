cmake_minimum_required(VERSION 3.10)

include(../../../config/cmake/create_targets.cmake)

make_library(pat pat)
target_link_libraries(pat PUBLIC "Qt6::Core;symbol")
target_include_directories(pat PUBLIC ${Boost_INCLUDE_DIR})

if(NOT IOS)
  make_qtest(test-pat-macro test/pep/parser_macro.test.cpp "pat;Qt6::Core;Qt6::Test;symbol")
  make_qtest(test-pat-lines test/pep/parser_lines.test.cpp "pat;Qt6::Core;Qt6::Test;symbol")
  make_qtest(test-pat-pep-from-tree-comment test/pep/ast/visitor/from_parse_tree/comment.test.cpp "pat;Qt6::Core;Qt6::Test;symbol")
  make_qtest(test-pat-pep-arg-from-parse-tree test/pep/ast/visitor/arg_from_parse_tree.test.cpp "pat;Qt6::Core;Qt6::Test;symbol")
  set_target_properties(test-pat-pep-arg-from-parse-tree PROPERTIES FOLDER "qtc_runnable")
endif()
