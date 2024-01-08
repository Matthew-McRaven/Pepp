file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/src/*.hpp")

make_target(
  TARGET demo-antlr
  TYPE "EXEC"
  SOURCES ${sources}
  DEPENDS Qt6::Core Qt6::Quick Qt6::Quick antlr4_shared parse
)

set_target_properties(demo-antlr PROPERTIES FOLDER "qtc_runnable")
