file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")

make_library2(
  TARGET isa
  SOURCES ${sources}
  DEPENDS Qt6::Core
)
