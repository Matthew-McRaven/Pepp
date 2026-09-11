file(GLOB srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

# Uses Qt's resources system for access to the test data.
qt6_add_executable(test-svgdom ${srcs}
                   "${PROJECT_SOURCE_DIR}/config/cmake/main.cpp")
target_link_libraries(test-svgdom PRIVATE pepp-lib catch Qt6::Core)
set_target_properties(test-svgdom PROPERTIES FOLDER "qtc_runnable")

if(NOT IOS)
  # WASM execution requires an extra shim script
  if(EMSCRIPTEN)
    add_test(
      NAME test-svgdom
      COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} "${PEPP_WASM_TEST_RUNNER}"
              $<TARGET_FILE:test-svgdom> ${PEPP_TESTS_SKIP_THROWS_ARGS})
  else()
    add_test(NAME test-svgdom COMMAND test-svgdom
                                        ${PEPP_TESTS_SKIP_THROWS_ARGS})
  endif()
endif()
