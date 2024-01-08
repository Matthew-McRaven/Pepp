file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")

make_library2(
  TARGET obj
  SOURCES ${sources}
  DEPENDS Qt6::Core elfio bits isa
)
