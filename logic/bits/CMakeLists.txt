try_compile(BITS_HAS_RANGES_REVERSE ${CMAKE_BINARY_DIR}/compile_tests ${CMAKE_CURRENT_LIST_DIR}/compile-test/ranges_reverse.cpp)
if (BITS_HAS_RANGES_REVERSE)
    message("Using std::ranges::reverse for byteswap")
    add_compile_definitions(BITS_HAS_RANGES_REVERSE)
else ()
    message("Using Qt helpers for byteswap")
endif ()

file(GLOB_RECURSE sources_and_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
file(GLOB_RECURSE tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/test/*.hpp")
file(GLOB_RECURSE compile_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/compile-test/*.cpp")
list(APPEND sources ${sources_and_tests})
list(REMOVE_ITEM sources ${tests})
list(REMOVE_ITEM sources ${compile_tests})

make_library2(
  TARGET bits
  SOURCES ${sources}
  DEPENDS Qt6::Core
  TESTS ${tests}
)
