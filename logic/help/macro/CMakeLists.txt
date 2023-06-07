make_library(macro macro)
target_include_directories(macro PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(macro PUBLIC "Qt6::Core;${Boost_LIBRARIES}")
target_compile_definitions(macro PRIVATE MACRO_LIBRARY)

if(NOT IOS)
  make_qtest(test-macro-registry test/registry.cpp "macro;Qt6::Core;${Boost_LIBRARIES};Qt6::Test")
  make_qtest(test-macro-parse test/parse.cpp "macro;Qt6::Core;${Boost_LIBRARIES};Qt6::Test")
endif()
