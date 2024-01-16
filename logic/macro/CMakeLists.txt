file(GLOB_RECURSE sources_and_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
file(GLOB_RECURSE tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/*.cpp")
list(APPEND sources ${sources_and_tests})
list(REMOVE_ITEM sources ${tests})

make_library(
  TARGET macro
  SOURCES ${sources}
  DEPENDS Qt6::Core antlr4_static
  TESTS ${tests}
)

# Will fail to find hpp files without this, see https://bugreports.qt.io/browse/QTBUG-101146.
get_target_property(ANTLR4_INCLUDE_DIR antlr4_static INCLUDE_DIRECTORIES)
target_include_directories(macro PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${ANTLR4_INCLUDE_DIR})
