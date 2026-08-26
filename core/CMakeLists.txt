# Build this library statically, because it will be consumed in two different
# SOs The first is the actual pepplib library, and the other is nanobind.
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
add_library(pepp-core STATIC ${sources})
set_target_properties(pepp-core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(
  pepp-core
  PUBLIC fmt
         spdlog
         cxxgraph
         flat
         kitty
         mockturtle
         elfio
         zpp_bits
         cnl)
target_include_directories(pepp-core PUBLIC ${CMAKE_CURRENT_LIST_DIR})

if(PEPP_PRECOMPILE_HEADERS)
  # Precompile elements of the standard library.
  pepp_precompile_headers(
    pepp-core
    PRIVATE
    <string>
    <memory>
    <vector>
    <map>
    <optional>
    <algorithm>
    <stdexcept>
    <array>)
  # Precompile most expensive headers from this library.
  pepp_precompile_headers(
    pepp-core PRIVATE "${CMAKE_CURRENT_LIST_DIR}/core/math/geom/interval.hpp")
endif()
