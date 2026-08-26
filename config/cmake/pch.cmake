include_guard()

# Wrapper around target_precompile_headers that restricts the PCH to C++.
#
# `project()` enables C as well as CXX, so CMake emits a C variant of the
# precompiled header (cmake_pch.h) next to the C++ one. Given that our code in
# PCHs are exclusively C++, we get compile errors.
#
# Restricting each entry to $<COMPILE_LANGUAGE:CXX> makes CMake skip the C
# variant altogether. Angle-bracket names need $<ANGLE-R> for their closing
# bracket, otherwise it terminates the generator expression early.
#
# Usage: pepp_precompile_headers(<target> <PRIVATE|PUBLIC|INTERFACE>
# <headers...>)
function(pepp_precompile_headers target visibility)
  if(NOT PEPP_PRECOMPILE_HEADERS)
    return()
  endif()
  set(guarded "")
  foreach(hdr IN LISTS ARGN)
    if(hdr MATCHES "^<(.+)>$")
      list(APPEND guarded
           "$<$<COMPILE_LANGUAGE:CXX>:<${CMAKE_MATCH_1}$<ANGLE-R>>")
    else()
      list(APPEND guarded "$<$<COMPILE_LANGUAGE:CXX>:${hdr}>")
    endif()
  endforeach()
  target_precompile_headers(${target} ${visibility} ${guarded})
endfunction()
