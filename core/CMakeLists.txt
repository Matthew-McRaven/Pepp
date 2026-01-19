# Build this library statically, because it will be consumed in two different
# SOs The first is the actual pepplib library, and the other is nanobind.
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
add_library(pepp-core STATIC ${sources})

target_link_libraries(pepp-core PUBLIC fmt spdlog cxxgraph flat)
target_include_directories(pepp-core PUBLIC ${CMAKE_CURRENT_LIST_DIR})
