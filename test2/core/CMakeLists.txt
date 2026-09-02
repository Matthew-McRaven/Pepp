file(GLOB_RECURSE srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

# Not sharded, because individual tests are cheap. Process start time would
# dominate runtime.
add_executable(test-core ${srcs})
target_link_libraries(test-core PRIVATE pepp-core catch-main)
set_target_properties(test-core PROPERTIES FOLDER "qtc_runnable")
pepp_precompile_headers(
  test-core
  PRIVATE
  <vector>
  <array>
  <string>
  <memory>
  <span>
  <cstdint>)

if(NOT (EMSCRIPTEN OR IOS))
  add_test(NAME test-core COMMAND test-core)
  # See test2/verify-asmb for why the slow entries carry an explicit COST.
  set_tests_properties(test-core PROPERTIES COST 9)
endif()
