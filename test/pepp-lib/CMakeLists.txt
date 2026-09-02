file(GLOB_RECURSE srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

# Not sharded, because individual tests are cheap. Process start time would
# dominate runtime. Tests in the root of this directory probably belong in
# core/, but they either have a as-of-yet-unremoved dependence on Qt or
# pepp-lib.
qt6_add_executable(test-pepplib ${srcs}
                   "${PROJECT_SOURCE_DIR}/config/cmake/main.cpp")
target_link_libraries(test-pepplib PRIVATE pepp-lib catch Qt6::Core)
set_target_properties(test-pepplib PROPERTIES FOLDER "qtc_runnable")
pepp_precompile_headers(test-pepplib PRIVATE <QtCore>)

if(NOT IOS)
  # WASM execution requires an extra shim
  if(EMSCRIPTEN)
    add_test(
      NAME test-pepplib
      COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} "${PEPP_WASM_TEST_RUNNER}"
              $<TARGET_FILE:test-pepplib> ${PEPP_TESTS_SKIP_THROWS_ARGS})
  else()
    add_test(NAME test-pepplib COMMAND test-pepplib
                                       ${PEPP_TESTS_SKIP_THROWS_ARGS})
  endif()
  set_tests_properties(test-pepplib PROPERTIES COST 9)
endif()
