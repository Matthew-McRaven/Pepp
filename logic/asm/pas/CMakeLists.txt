file(GLOB_RECURSE sources_and_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
file(GLOB_RECURSE tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/*.cpp")
list(APPEND sources ${sources_and_tests})
list(REMOVE_ITEM sources ${tests})

make_library2(
  TARGET pas
  SOURCES ${sources}
  DEPENDS Qt6::Core bits elfio symbol macro isa obj
  TESTS ${tests}
  TEST_DEPENDS builtins targets
)
target_include_directories(pas PUBLIC ${Boost_INCLUDE_DIR})
