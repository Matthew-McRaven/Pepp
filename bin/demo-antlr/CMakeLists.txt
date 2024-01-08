file(GLOB_RECURSE antlr-sources CONFIGURE_DEPENDS "antlr/*.h" "antlr/*.cpp")
add_library(demo-antlr-generated OBJECT ${antlr-sources})

# Will fail to find hpp files without this, see https://bugreports.qt.io/browse/QTBUG-101146.
get_target_property(ANTLR4_INCLUDE_DIR antlr4_shared INCLUDE_DIRECTORIES)
target_include_directories(demo-antlr-generated PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${ANTLR4_INCLUDE_DIR} ${CMAKE_CURRENT_LIST_DIR}/antlr)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/src/*.hpp")

make_target(
  TARGET demo-antlr
  TYPE "EXEC"
  SOURCES ${sources}
  DEPENDS Qt6::Core Qt6::Quick Qt6::Quick demo-antlr-generated antlr4_shared demo-antlr-generated
)

set_target_properties(demo-antlr PROPERTIES FOLDER "qtc_runnable")
