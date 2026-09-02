file(GLOB srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

# Uses Qt's resources system for access to the test data.
qt6_add_executable(test-builtins ${srcs}
                   "${PROJECT_SOURCE_DIR}/config/cmake/main.cpp")
target_link_libraries(test-builtins PRIVATE pepp-lib catch Qt6::Core)
set_target_properties(test-builtins PROPERTIES FOLDER "qtc_runnable")

if(NOT IOS)
  # WASM execution requires an extra shim script
  if(EMSCRIPTEN)
    add_test(
      NAME test-builtins
      COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} "${PEPP_WASM_TEST_RUNNER}"
              $<TARGET_FILE:test-builtins> ${PEPP_TESTS_SKIP_THROWS_ARGS})
  else()
    add_test(NAME test-builtins COMMAND test-builtins
                                        ${PEPP_TESTS_SKIP_THROWS_ARGS})
  endif()
endif()
