cmake_minimum_required(VERSION 3.10)

# set the project name
project(test-execution-breakpoints-Pep10 VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Allow version header to be included
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "**/*.cpp" "*.cpp")
add_executable(test-execution-breakpoints-Pep10 ${sources})

if (code_coverage)
	# Add code coverage support
	SET(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
	SET(CMAKE_C_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
endif ()


target_link_libraries(test-execution-breakpoints-Pep10 RTL-Lib catch magic_enum builtins)
target_link_libraries(test-execution-breakpoints-Pep10 machine)

add_test(NAME test-execution-breakpoints-Pep10 COMMAND test-execution-breakpoints-Pep10 -r junit --out junit.xml)
