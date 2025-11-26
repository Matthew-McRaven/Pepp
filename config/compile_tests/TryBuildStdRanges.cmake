# Must be earlier or we have issues creating installers.
try_compile(PEPP_HAS_RANGES_REVERSE ${CMAKE_BINARY_DIR}/compile_tests
            ${CMAKE_CURRENT_LIST_DIR}/ranges_reverse.cpp)
if(PEPP_HAS_RANGES_REVERSE)
  message("Using std::ranges::reverse for byteswap")
  add_compile_definitions(PEPP_HAS_RANGES_REVERSE)
else()
  message("Using Qt helpers for byteswap")
endif()
