cmake_minimum_required(VERSION 3.10)

try_compile(BITS_HAS_RANGES_REVERSE ${CMAKE_BINARY_DIR}/compile_tests ${CMAKE_CURRENT_LIST_DIR}/compile-test/ranges_reverse.cpp)
if(BITS_HAS_RANGES_REVERSE)
    message("Using std::ranges::reverse for byteswap")
else()
    message("Using Qt helpers for byteswap")
endif()

make_library(bits bits)
target_link_libraries(bits PUBLIC "Qt6::Core")
target_include_directories(bits PUBLIC ${Boost_INCLUDE_DIR})

if(NOT IOS)
  make_qtest(test-bits-copy test/copy.test.cpp "bits;Qt6::Core;Qt6::Test")
  make_qtest(test-bits-log2 test/log2.test.cpp "bits;Qt6::Core;Qt6::Test")
  make_qtest(test-bits-strings-ascii-hex test/strings_ascii_hex.test.cpp "bits;Qt6::Core;Qt6::Test")
  set_target_properties(test-bits-strings-ascii-hex PROPERTIES FOLDER "qtc_runnable")
endif()
