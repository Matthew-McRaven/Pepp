cmake_minimum_required(VERSION 3.10)

# set the project name
project(test-isa-prevalidation-Pep10 VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Allow version header to be included
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "**/*.cpp" "*.cpp")
add_executable (test-isa-prevalidation-Pep10 ${sources})

if(code_coverage)
	# Add code coverage support
	SET(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
	SET(CMAKE_C_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
endif()
if(EMSCRIPTEN)
	set_target_properties(test-isa-prevalidation-Pep10 PROPERTIES LINK_FLAGS " -s DISABLE_EXCEPTION_CATCHING=0 -s ALLOW_MEMORY_GROWTH=1")
	# Dtest-isable unit tests that throw exceptions, because exceptions can't be caught inside wasm.
	target_compile_definitions(test-isa-prevalidation-Pep10 PRIVATE EMSCRIPTEN=1)
endif()


target_link_libraries(test-isa-prevalidation-Pep10 RTL-Lib catch magic_enum builtins)

add_test (NAME test-isa-prevalidation-Pep10 COMMAND test-isa-prevalidation-Pep10 -r junit --out junit.xml)