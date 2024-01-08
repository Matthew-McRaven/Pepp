# Will fail to find hpp files without this, see https://bugreports.qt.io/browse/QTBUG-101146.


file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp" "${CMAKE_CURRENT_LIST_DIR}/*.h")

make_target(
  TARGET parse
  SOURCES ${sources}
  DEPENDS Qt6::Core antlr4_shared
)

get_target_property(ANTLR4_INCLUDE_DIR antlr4_shared INCLUDE_DIRECTORIES)
target_include_directories(parse PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${ANTLR4_INCLUDE_DIR})
#set_target_properties(demo-antlr PROPERTIES FOLDER "qtc_runnable")
