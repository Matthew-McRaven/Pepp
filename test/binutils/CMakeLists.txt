# Compares pepp's version of binutils with GNU's. Each test finds its GNU tool
# on PATH or through an override such as $PEPP_TEST_AR. Tests are skipped if no
# compatible tool is found.
if(NOT (IOS OR EMSCRIPTEN) AND Python_FOUND)
  add_test(NAME binutils-ar COMMAND ${Python_EXECUTABLE} -I
                                    "${CMAKE_CURRENT_LIST_DIR}/test_ar.py")
  set_tests_properties(
    binutils-ar
    PROPERTIES
      ENVIRONMENT
      "PEPP_TERM=$<TARGET_FILE:pepp-term>;PEPP_AR_MEMBERS=${PROJECT_SOURCE_DIR}/test/core-data/formats/ar/data/regular.a"
      SKIP_REGULAR_EXPRESSION
      "no GNU ar found")
endif()
