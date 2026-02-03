# Helper to find and enable clang-tidy
macro(inject_clang_tidy)
  # Run clang tidy iff it is installed and on the path. find_program(CLANG_TIDY
  # "clang-tidy") set(CMAKE_CXX_CLANG_TIDY clang-tidy
  # -checks=-*,clang-analyzer-*)
endmacro()

# Add code coverage support, must come before target definition.
macro(inject_code_coverage enable_coverage)
  message(STATUS "Code coverage enabled: ${enable_coverage}")
  if(${enable_coverage})
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate -fcoverage-mapping)
  endif()
endmacro()

macro(inject_dwarf_debug)
  if(!MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gdwarf-4")
  endif()
endmacro()
