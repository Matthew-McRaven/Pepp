cmake_minimum_required(VERSION 3.10)

# set the project name
project(test-build-examples VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Allow version header to be included
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "**/*.cpp" "*.cpp")
add_executable (test-build-examples ${sources})

 if(code_coverage)
	# Add code coverage support
	SET(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
	SET(CMAKE_C_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
endif()
if(EMSCRIPTEN)
	set_target_properties(test-build-examples PROPERTIES LINK_FLAGS " -s DISABLE_EXCEPTION_CATCHING=0 -s ALLOW_MEMORY_GROWTH=1")
	# Disable unit tests that throw exceptions, because exceptions can't be caught inside wasm.
	target_compile_definitions(test-build-examples PRIVATE EMSCRIPTEN=1)
endif()


target_link_libraries(test-build-examples RTL-Lib catch magic_enum builtins)

add_test (NAME test-build-examples COMMAND test-build-examples -r junit --out junit.xml)