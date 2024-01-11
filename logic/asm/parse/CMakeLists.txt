

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp" "${CMAKE_CURRENT_LIST_DIR}/*.h")
file(GLOB_RECURSE tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/test/*.hpp")
list(APPEND sources ${sources_and_tests})
list(REMOVE_ITEM sources ${tests})

make_library(
  TARGET parse
  SOURCES ${sources}
  DEPENDS Qt6::Core antlr4_shared
  TESTS ${tests}
  TEST_IN_QTC
  TEST_DEPENDS catch
)


# Will fail to find hpp files without this, see https://bugreports.qt.io/browse/QTBUG-101146.
get_target_property(ANTLR4_INCLUDE_DIR antlr4_shared INCLUDE_DIRECTORIES)
target_include_directories(parse PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${ANTLR4_INCLUDE_DIR})
