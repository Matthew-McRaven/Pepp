
# Helper to find and enable clang-tidy
macro(inject_clang_tidy)
  # Run clang tidy iff it is installed and on the path. find_program(CLANG_TIDY
  # "clang-tidy") set(CMAKE_CXX_CLANG_TIDY clang-tidy
  # -checks=-*,clang-analyzer-*)
endmacro()

# Force our desired C++ standard.
macro(inject_cxx_standard)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED True)
endmacro()

# Add code coverage support, must come before target definition.
macro(inject_code_coverage)
  if(${code_coverage})
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
  endif()
endmacro()

macro(inject_dwarf_debug)
  if(!MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gdwarf-4")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-ftime-trace)
  endif()
endmacro()
