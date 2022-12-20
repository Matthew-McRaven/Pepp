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
macro(make_target target_name TYPE)

    inject_cxx_standard()
    inject_clang_tidy()
    inject_code_coverage()

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gdwarf-4")

    # PUBLIC is not a valid visibility for libraries, so must exclude that TYPE.
    if (${TYPE} STREQUAL PUBLIC)
        add_library(${target_name} ${sources})
    else ()
        add_library(${target_name} ${TYPE} ${sources})
    endif ()

    # Mark src/ as the root from where includes should take place.
    target_include_directories(${target_name} ${TYPE} ${CMAKE_CURRENT_LIST_DIR}/src)
    # And always link against boost...
    target_link_libraries(${target_name} ${TYPE} ${Boost_LIBRARIES})

endMacro()

# Helper to make a PUBLIC library with cpp sources.
macro(make_library target_name root)
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/${root}/**/*.cpp" "src/${root}/*.cpp")
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

# Helper to create a unit test program for unit tests.
# Assumes that it is being called from the root of a library, so tests reside in ./test.
macro(make_napi_module target_name root bitness)
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/${root}/**/*.cpp" "src/${root}/*.cpp")
    inject_cxx_standard()
    inject_code_coverage()

    # Include N-API wrappers
    execute_process(COMMAND node -p "require('node-addon-api').include"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE NODE_ADDON_API_DIR
            )
    # Node injects a \n, we must remove. See: https://stackoverflow.com/questions/56288848/unix-make-file-fails-with-cmakejs-when-adding-a-dependency
    string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
    string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

    # Include Node headers
    execute_process(COMMAND npm config get prefix
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE TARGET_NODE_ROOT
            )
    # NPM isn't always available...
    if (NOT "${TARGET_NODE_ROOT}" STREQUAL "")
        # Node injects a \n, we must remove. See: https://stackoverflow.com/questions/56288848/unix-make-file-fails-with-cmakejs-when-adding-a-dependency
        string(REPLACE "\n" "" TARGET_NODE_ROOT ${TARGET_NODE_ROOT})
        string(REPLACE "\"" "" TARGET_NODE_ROOT ${TARGET_NODE_ROOT})
    endif ()

    # Include other possible Node headers by getting the node executable dir
    find_program(NODE_BIN_LOCATION node)
    cmake_path(GET NODE_BIN_LOCATION PARENT_PATH NODE_PARENT_DIR)

    # Add any files given to us by cmake-js, these are likely to be empty.
    add_library(${target_name} SHARED ${sources} ${CMAKE_JS_SRC})

    target_include_directories(${target_name} PRIVATE ${CMAKE_JS_INC} ${NODE_ADDON_API_DIR} ${TARGET_NODE_ROOT}/include/node ${NODE_PARENT_DIR}/../include/node)
    set_target_properties(${target_name} PROPERTIES PREFIX "" SUFFIX ".node")

    # Define NAPI_VERSION
    add_definitions(-DNAPI_VERSION=6)

    # Handle defines which handle different bitness of libraries
    target_compile_definitions(${target_name} PUBLIC -DADDR_TYPE=uint${bitness}_t)
    target_compile_definitions(${target_name} PUBLIC -DADDR_SUFFIX="u${bitness}")

    # Copy .node files to dist/, where they will be uploaded.
    add_custom_command(
            TARGET ${target_name} PRE_LINK
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_LIST_DIR}/dist)
    add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:${target_name}>
            ${CMAKE_CURRENT_LIST_DIR}/dist)
endmacro()
