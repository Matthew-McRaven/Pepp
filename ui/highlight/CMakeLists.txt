file(GLOB_RECURSE sources_and_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
file(GLOB_RECURSE tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/test/*.hpp")
file(GLOB_RECURSE old_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/__old/*.cpp")
list(APPEND sources ${sources_and_tests})
list(REMOVE_ITEM sources ${tests})
list(REMOVE_ITEM sources ${old_sources})

make_library2(
  TARGET highlight
  SOURCES ${sources}
  DEPENDS Qt6::Core Qt6::Gui Qt6::Quick isa
  TESTS ${tests}
)
