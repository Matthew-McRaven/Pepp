file(GLOB_RECURSE srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

# Core tests that read data files through Qt's resource system.
qt6_add_executable(test-binutils ${srcs}
                   "${PROJECT_SOURCE_DIR}/config/cmake/main.cpp")
target_link_libraries(test-binutils PRIVATE pepp-core catch Qt6::Core)
set_target_properties(test-binutils PROPERTIES FOLDER "qtc_runnable")

qt_add_resources(
  test-binutils
  "ar_data"
  PREFIX
  "/ar"
  BASE
  "${CMAKE_CURRENT_LIST_DIR}/data/ar-mixed-members"
  FILES
  data/ar-mixed-members/regular.a
  data/ar-mixed-members/thin.a)

if(NOT IOS)
  # WASM execution requires an extra shim script
  if(EMSCRIPTEN)
    add_test(
      NAME test-binutils
      COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} "${PEPP_WASM_TEST_RUNNER}"
              $<TARGET_FILE:test-binutils> ${PEPP_TESTS_SKIP_THROWS_ARGS})
  else()
    add_test(NAME test-binutils COMMAND test-binutils
                                        ${PEPP_TESTS_SKIP_THROWS_ARGS})
  endif()
endif()

# Compares pepp's version of binutils with GNU's. Each test finds its GNU tool
# on PATH or through an override such as $PEPP_TEST_AR. Tests are skipped if no
# compatible tool is found.
if(NOT (IOS OR EMSCRIPTEN) AND Python_FOUND)
  add_test(NAME binutils-ar COMMAND ${Python_EXECUTABLE} -I
                                    "${CMAKE_CURRENT_LIST_DIR}/test_ar.py")
  set_tests_properties(
    binutils-ar PROPERTIES ENVIRONMENT "PEPP_TERM=$<TARGET_FILE:pepp-term>"
                           SKIP_REGULAR_EXPRESSION "no GNU ar found")
  add_test(NAME binutils-ld COMMAND ${Python_EXECUTABLE} -I
                                    "${CMAKE_CURRENT_LIST_DIR}/test_ld.py")
  set_tests_properties(
    binutils-ld PROPERTIES ENVIRONMENT "PEPP_TERM=$<TARGET_FILE:pepp-term>")
endif()
