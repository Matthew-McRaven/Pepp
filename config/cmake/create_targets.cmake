include_guard()
# Allows us to not use __declspec(dll...) everywhere
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
# We always want universal builds, so do not set on a per-target basis
# NOTE: This prevents us from building statically!!
SET(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "" FORCE)


# test-lib-all will include the sources for all catch tests, and be dependent on all the tests' libaries.
# Our test browser must depend on this target.
qt6_add_library(test-lib-all INTERFACE)
# test-all bundles all the tests into a single executable to prove that there are no linker errors.
# do not add to ctest, otherwise every test runs twice.
qt6_add_executable(test-all ${CMAKE_CURRENT_LIST_DIR}/main.cpp)
set_target_properties(test-all PROPERTIES FOLDER "qtc_runnable")
target_link_libraries(test-all PUBLIC test-lib-all catch)
# Failure to copy DLLs on Windows causes tests to fail at runtime.
if (WIN32)
    add_custom_command(TARGET test-all POST_BUILD
            COMMAND ${CMAKE_COMMAND}
            ARGS -E copy $<TARGET_RUNTIME_DLLS:test-all> $<TARGET_FILE_DIR:test-all>
            COMMAND_EXPAND_LISTS
    )
endif ()

# Track all libraries that have been linked to in make_target.
# At the end of all build steps we can copy all of the libraries exactly once.
DEFINE_PROPERTY(GLOBAL PROPERTY ALL_LIBRARIES)
set_property(GLOBAL PROPERTY ALL_LIBRARIES "")

function(catch_test_count count_name)
    get_target_property(FILES test-lib-all INTERFACE_SOURCES)
    list(LENGTH FILES count)
    set(${count_name} ${count} PARENT_SCOPE)
endfunction()

function(maybe_append_all_libraries library)
    # message("Called with ${library}")
    # Check if the custom property (LINKED_LIBRARIES) exists for the target.
    get_property(is_defined GLOBAL PROPERTY ALL_LIBRARIES DEFINED)
    if (is_defined)
        # Property exists, check if the library is already in the list.
        get_property(linked_libraries GLOBAL PROPERTY ALL_LIBRARIES)
        # message("So far: ${linked_libraries}")
        if (NOT "${library}" IN_LIST linked_libraries)
            # message("So far ${linked_libraries}")
            # Library not found in the list, need to link it.
            target_link_libraries(test-lib-all INTERFACE ${library})
            # Update the LINKED_LIBRARIES property.
            list(APPEND linked_libraries ${library})
            set_property(GLOBAL PROPERTY ALL_LIBRARIES ${linked_libraries})
        endif ()
    else ()
        # Property does not exist, so link the library and create the property.
        target_link_libraries(test-lib-all INTERFACE ${library})
        set_property(GLOBAL PROPERTY ALL_LIBRARIES ${library})
        # message("It is ${is_defined}")
    endif ()
endfunction()


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


function(make_target)
    inject_cxx_standard()
    inject_clang_tidy()
    inject_code_coverage()
    # Both TYPE and TARGET are required strings.
    set(oneValueArgs TYPE TARGET)
    # SOURCES are required and DEPENDS is optional.
    # If DEPENDS is not present, I will not call target_link_libraries.
    set(multiValueArgs SOURCES DEPENDS)
    cmake_parse_arguments("MK" "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if (("TARGET" IN_LIST MK_KEYWORDS_MISSING_VALUES) OR (NOT DEFINED MK_TARGET))
        message(FATAL_ERROR "TARGET not defined ${MK_KEYWORDS_MISSING_VALUES}")
    endif ()

    if (!MSVC)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gdwarf-4")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-ftime-trace)
    endif ()

    # Ensure target type is set, default to public
    if (("TYPE" IN_LIST MK_KEYWORDS_MISSING_VALUES) OR (NOT DEFINED MK_TYPE))
        message(WARNING "TYPE not defined for target ${MK_TARGET}")
        set(MK_TYPE "PUBLIC")
    endif ()

    # PUBLIC is not a valid visibility for libraries, so must exclude that TYPE.
    if (MK_TYPE STREQUAL PUBLIC)
        qt6_add_library(${MK_TARGET} ${MK_SOURCES})
    elseif (MK_TYPE STREQUAL EXEC)
        qt6_add_executable(${MK_TARGET} ${MK_SOURCES})
        # Replaces old make_qtest macro whose only difference was DLL copying and add_test
    elseif (MK_TYPE STREQUAL TEST)
        # Amalgamate all test sources into a single target, and link against all dependencies exactly once.
        target_sources(test-lib-all INTERFACE ${MK_SOURCES})
        foreach (depend ${MK_DEPENDS})
            # Don't bring in another source of "main"
            if (NOT ${depend} MATCHES "catch-main")
                maybe_append_all_libraries(${depend})
            endif ()
        endforeach ()
        # We don't actually generate a target, so must return early to avoid linking this target.
        return()
    else ()
        qt6_add_library(${MK_TARGET} ${MK_TYPE} ${MK_SOURCES})
    endif ()

    # Only add dependencies if present
    if (MK_DEPENDS)
        target_link_libraries(${MK_TARGET} PUBLIC ${MK_DEPENDS})
    endif ()

    install(TARGETS ${MK_TARGET}
            BUNDLE DESTINATION .
            RUNTIME DESTINATION bin
    )
endfunction()

# Helper to make a PUBLIC library with cpp sources.
function(make_library)
    set(options TEST_IN_QTC)
    set(oneValueArgs TARGET TYPE)
    # SOURCES and DEPENDS work as above.
    # TESTS should be a list of standalone CPP files that can be compiled into a test executable
    # These tests will be linked against the target library,  QTest, and and additional TEST_DEPENDS libraries.
    set(multiValueArgs SOURCES DEPENDS TESTS TEST_DEPENDS)
    cmake_parse_arguments(ML "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Ensure target type is set, default to public
    if (("TYPE" IN_LIST ML_KEYWORDS_MISSING_VALUES) OR (NOT DEFINED ML_TYPE))
        set(ML_TYPE "PUBLIC")
    endif ()

    make_target(
            TARGET "${ML_TARGET}"
            TYPE ${ML_TYPE}
            SOURCES ${ML_SOURCES}
            DEPENDS ${ML_DEPENDS}
    )

    string(TOUPPER ${ML_TARGET} ML_TARGET_UPPER)
    target_compile_definitions(${ML_TARGET} PRIVATE "${ML_TARGET_UPPER}_LIBRARY")
    # Make target for each test file
    foreach (TEST_FILE ${ML_TESTS})
        get_filename_component(STEM ${TEST_FILE} NAME_WE)
        make_target(
                TARGET "test-${ML_TARGET}-${STEM}"
                TYPE "TEST"
                SOURCES ${TEST_FILE}
                DEPENDS ${ML_DEPENDS} ${ML_TARGET} ${ML_TEST_DEPENDS}
        )
    endforeach ()
endfunction()
