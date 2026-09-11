# Must be earlier or we have issues creating installers.
try_compile(PEPP_HAS_DOUBLE_FROM_CHARS ${CMAKE_BINARY_DIR}/compile_tests
            ${CMAKE_CURRENT_LIST_DIR}/fromchars_double.cpp)
if(PEPP_HAS_DOUBLE_FROM_CHARS)
  message("Allow std::from_chars for doubles")
  add_compile_definitions(PEPP_HAS_DOUBLE_FROM_CHARS)
else()
  message("Use fallback to parse doubles")
endif()
