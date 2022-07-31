cmake_minimum_required(VERSION 3.10)

# set the project name
project(test-isa-definitions VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Allow version header to be included
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "**/*.cpp" "*.cpp")
add_executable(test-isa-definitions ${sources})

if (code_coverage)
    # Add code coverage support
    SET(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
    SET(CMAKE_C_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
endif ()


target_link_libraries(test-isa-definitions RTL-Lib catch magic_enum builtins)
target_link_libraries(test-isa-definitions machine)

add_test(NAME test-isa-definitions COMMAND test-isa-definitions -r junit --out junit.xml)
