file(GLOB_RECURSE tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

get_filename_component(CURRENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
target_sources(test-lib-all INTERFACE ${tests})
maybe_append_all_libraries(pepp-lib)

if(MOCKTURTLE_BUILD_TESTS)
  add_test(NAME all-mockturtle-tests COMMAND run_mockturtle_tests)
endif()
