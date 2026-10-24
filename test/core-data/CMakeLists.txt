file(GLOB_RECURSE srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

# Core tests that read data files through Qt's resource system.
qt6_add_executable(test-core-data ${srcs}
                   "${PROJECT_SOURCE_DIR}/config/cmake/main.cpp")
target_link_libraries(test-core-data PRIVATE pepp-core catch Qt6::Core)
set_target_properties(test-core-data PROPERTIES FOLDER "qtc_runnable")

qt_add_resources(
  test-core-data
  "ar_data"
  PREFIX
  "/ar"
  BASE
  "${CMAKE_CURRENT_LIST_DIR}/formats/ar/data"
  FILES
  formats/ar/data/regular.a
  formats/ar/data/thin.a)

if(NOT IOS)
  # WASM execution requires an extra shim script
  if(EMSCRIPTEN)
    add_test(
      NAME test-core-data
      COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} "${PEPP_WASM_TEST_RUNNER}"
              $<TARGET_FILE:test-core-data> ${PEPP_TESTS_SKIP_THROWS_ARGS})
  else()
    add_test(NAME test-core-data COMMAND test-core-data
                                         ${PEPP_TESTS_SKIP_THROWS_ARGS})
  endif()
endif()
