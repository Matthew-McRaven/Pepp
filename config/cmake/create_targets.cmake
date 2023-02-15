include_guard()

# We always want universal builds, so do not set on a per-target basis
SET(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "" FORCE)

# Helper to find and enable clang-tidy
macro(inject_clang_tidy)
    # Run clang tidy iff it is installed and on the path.
    # find_program(CLANG_TIDY "clang-tidy")
    # set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,clang-analyzer-*)
endmacro()

# Force our desired C++ standard.
macro(inject_cxx_standard)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED True)
endmacro()


# Add code coverage support, must come before target definition.
macro(inject_code_coverage)
    if (${code_coverage})
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    endif ()
endmacro()


# Helper that can be used to create either an interface or shared library
# Variable "sources" must have the list of files you want included in the library.
macro(make_exec target_name root)
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/${root}/**/*.cpp" "src/${root}/*.cpp"
      "src/${root}/**/*.hpp" "src/${root}/*.hpp")
    inject_cxx_standard()
    inject_clang_tidy()
    inject_code_coverage()

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gdwarf-4")

    qt_add_executable(${target_name} ${sources})

endMacro()

# Helper that can be used to create either an interface or shared library
# Variable "sources" must have the list of files you want included in the library.
macro(make_target target_name TYPE)

    inject_cxx_standard()
    inject_clang_tidy()
    inject_code_coverage()

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gdwarf-4")

    # PUBLIC is not a valid visibility for libraries, so must exclude that TYPE.
    if (${TYPE} STREQUAL PUBLIC)
        qt6_add_library(${target_name} ${sources})
    else ()
        qt6_add_library(${target_name} ${TYPE} ${sources})
    endif ()

    # Mark src/ as the root from where includes should take place.
    target_include_directories(${target_name} ${TYPE} ${CMAKE_CURRENT_LIST_DIR}/src)
    # And always link against boost...
    # target_link_libraries(${target_name} ${TYPE} ${Boost_LIBRARIES})

endMacro()

# Helper to make a PUBLIC library with cpp sources.
macro(make_library target_name root)
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/${root}/**/*.cpp" "src/${root}/*.cpp"
        "src/${root}/**/*.hpp" "src/${root}/*.hpp")
    make_target(${target_name} PUBLIC)
endMacro()

# Helper to make an INTERFACE library with HPP sources (headers).
macro(make_interface target_name)
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/${root}/**/*.hpp")
    make_target(${target_name} INTERFACE)
endMacro()

# Helper to create a unit test program for unit tests.
# Assumes that it is being called from the root of a library, so tests reside in ./test.
macro(make_test target_name root dep)
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${root}/**/*.cpp" "${root}/*.cpp")

    if (sources)
        inject_cxx_standard()
        inject_code_coverage()

        add_executable(${target_name} ${sources})

        # Every test *should* use catch, so we will link against it here.
        target_link_libraries(${target_name} PRIVATE catch ${dep})
        # And run the test with the correct reporting options.
        add_test(NAME ${target_name} COMMAND ${target_name} -r junit --out junit.xml)
    endif ()
endmacro()

