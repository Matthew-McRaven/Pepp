# Build this library statically, because it will be consumed in two different
# SOs The first is the actual pepplib library, and the other is nanobind.
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
add_library(pepp-core STATIC ${CMAKE_CURRENT_LIST_DIR}/dummy.c)
# target_link_libraries(pepp-core PUBLIC fmt spdlog cxxgraph flat)
# target_compile_features(pepp-core PUBLIC cxx_std_20)
