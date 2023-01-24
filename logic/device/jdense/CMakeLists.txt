cmake_minimum_required(VERSION 3.10)

make_library(split-tb-zero .)
target_include_directories(split-tb-zero PUBLIC ${Boost_INCLUDE_DIRS} src)
target_link_libraries(split-tb-zero PRIVATE fmt iface)

make_test(split-tb-zero-test test split-tb-zero catch)