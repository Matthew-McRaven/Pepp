file(GLOB_RECURSE tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
add_pepp_test(TARGET bits TESTS ${tests})
