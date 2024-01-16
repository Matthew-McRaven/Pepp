file(GLOB_RECURSE sources_and_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
file(GLOB_RECURSE tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/test/*.hpp")
file(GLOB_RECURSE compile_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/compile-test/*.cpp")
list(APPEND sources ${sources_and_tests})
list(REMOVE_ITEM sources ${tests})
list(REMOVE_ITEM sources ${compile_tests})

make_target(
  TARGET pepp-term
  TYPE "EXEC"
  SOURCES ${sources}
  DEPENDS TextFlow cli about macro pas builtins isa targets sim Qt6::Core
)


# GLOB_RECURSE ueses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
set_target_properties(pepp-term PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)

# Non-DLL platforms have empty generator expressin, so use more generator magic to only copy DLLs if the exist.
add_custom_command(TARGET pepp-term POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS
        -E $<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:pepp-term>>,copy,true> $<TARGET_RUNTIME_DLLS:pepp-term> $<TARGET_FILE_DIR:pepp-term>
        COMMAND_EXPAND_LISTS
)


install(TARGETS pepp-term
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

set_target_properties(pepp-term PROPERTIES FOLDER "qtc_runnable")

set_property(SOURCE "test/impl.cpp.in" PROPERTY SKIP_AUTOMOC ON)

# Stop searching for python on first match.
cmake_policy(SET CMP0094 NEW)
# Prevent registry from pre-empting our python installation, needed in CI to find debug libs.
set(Python3_FIND_REGISTRY LAST)

find_package(Python 3.12 OPTIONAL_COMPONENTS Development Interpreter)

# Windows may report python found without debug libs present.
# Try actually compiling a program with python to see if we get link errors.
if(Python_Development_FOUND)
try_compile(PYTHON_LINK_SUCCESS
        ${CMAKE_CURRENT_BINARY_DIR}/python_link_test
        ${CMAKE_CURRENT_LIST_DIR}/compile-test/py_link_test.cpp
        LINK_LIBRARIES ${Python_LIBRARIES}
        OUTPUT_VARIABLE PYTHON_LINK_OUTPUT)
else()
  set(PYTHON_LINK_SUCCESS FALSE)
endif()

if (Python_Development_FOUND AND Python_Interpreter_FOUND AND PYTHON_LINK_SUCCESS AND ENABLE_TERM_TESTS)
    # Create file which contains PYTHONHOME followed by PYTHONPATH values seperated by newlines.
    add_custom_command(
        COMMAND ${Python_EXECUTABLE}
        ARGS ${CMAKE_CURRENT_LIST_DIR}/print_path.py
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pythonpath.txt"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    # Required logic to trigger python paths generation as part of build.
    add_custom_target(gen-python-paths ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/pythonpath.txt")

    add_dependencies(pepp-term gen-python-paths)

    # Files are generated in source tree because QT Creator had a hard time picking up on tests.

    # From a target name and a python script, create a QTest application.
    function(configure_cpp_pytest target PY_SCRIPT)
        get_filename_component(FILENAME ${PY_SCRIPT} NAME_WE)
        # NAME is lowercase, except first letter is upper
        string(SUBSTRING ${FILENAME} 0 1 FIRST_LETTER)
        string(TOUPPER ${FIRST_LETTER} FIRST_LETTER)
        string(REGEX REPLACE "^.(.*)" "${FIRST_LETTER}\\1" NAME "${FILENAME}")
        set(OUT_NAME_WE "${CMAKE_CURRENT_SOURCE_DIR}/test/generated/${FILENAME}")
        configure_file(test/impl.cpp.in ${OUT_NAME_WE}.cpp)
        make_target(
            TARGET ${target}
            TYPE "EXEC"
            SOURCES "${OUT_NAME_WE}.cpp"
            DEPENDS Qt6::Core Qt6::Test Python::Python)

        # Update working directory for scripts where it matters.
        get_filename_component(PY_WORKDIR ${PY_SCRIPT} DIRECTORY)
        set_target_properties(${target} PROPERTIES WORKING_DIRECTORY ${PY_WORKDIR})

        qt_add_resources(${target} "${target}"
            PREFIX "/py"
            FILES "${PY_SCRIPT}"
        )
        target_compile_definitions(${target} PRIVATE QT_NO_KEYWORDS)
        add_dependencies(${target} pepp-term)

        # And copy those test items to the output directory for ease of execution.
        add_custom_target("${target}-makedir"
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target}>/${FILENAME}
            COMMAND_EXPAND_LISTS
        )
        file(GENERATE OUTPUT $<TARGET_FILE_DIR:${target}>/${FILENAME}/test_config.txt CONTENT "$<TARGET_FILE:pepp-term>\n${CMAKE_CURRENT_BINARY_DIR}/pythonpath.txt")
        add_dependencies(${target} pepp-term "${target}-makedir")
    endfunction()



    function(configure_cpp_pyest_sample target sampledir)

        get_filename_component(FILENAME "${sampledir}" NAME_WE)
        # Dummy field needed by impl. This file doesn't really exist, but we will alias it to something that does.
        set(PY_SCRIPT "test/${FILENAME}.py")
        # NAME is lowercase, except first letter is upper
        string(SUBSTRING ${FILENAME} 0 1 FIRST_LETTER)
        string(TOUPPER ${FIRST_LETTER} FIRST_LETTER)
        string(REGEX REPLACE "^.(.*)" "${FIRST_LETTER}\\1" NAME "${FILENAME}")

        set(OUT_NAME_WE "${CMAKE_CURRENT_SOURCE_DIR}/test/generated/${FILENAME}")
        configure_file(test/impl.cpp.in ${OUT_NAME_WE}.cpp)
        # Duplicate run script so tht we can properly set alias
        # Must use gen_ prefix to output, or it could cause the TARGET_FILE to become a directory by accident.
        configure_file(test/sample_run.py.in ${CMAKE_CURRENT_BINARY_DIR}/gen_${target}/sample_run.py)
        set_property(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/gen_${target}/sample_run.py" PROPERTY QT_RESOURCE_ALIAS "test/${FILENAME}.py")

        make_target(
            TARGET ${target}
            TYPE "EXEC"
            SOURCES "${OUT_NAME_WE}.cpp"
            DEPENDS Qt6::Core Qt6::Test Python::Python)
        # Update working directory for scripts where it matters.
        set_target_properties(${target} PROPERTIES WORKING_DIRECTORY ${sampledir})

        qt_add_resources(${target} "${target}"
            PREFIX "/py"
            FILES "${CMAKE_CURRENT_BINARY_DIR}/gen_${target}/sample_run.py"
        )


        target_compile_definitions(${target} PRIVATE QT_NO_KEYWORDS)



        # Enumerate all files in sample dir, excluding the python script.
        file(GLOB_RECURSE samples CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/${sampledir}/*")
        list(REMOVE_ITEM samples run.py)

        # Prevent samples from being empty, cause difficult-to-debug errors.
        if (NOT (DEFINED samples AND NOT samples STREQUAL ""))
            message( SEND_ERROR "Samples must be non-empty for ${target}" )
        endif()

        # And copy those test items to the output directory for ease of execution.
        add_custom_target("${target}-makedir"
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target}>/${FILENAME}
            COMMAND_EXPAND_LISTS
        )
        add_custom_target("${target}-copyfiles"
            COMMAND ${CMAKE_COMMAND}
            -E copy ${samples} $<TARGET_FILE_DIR:${target}>/${FILENAME}
            COMMAND_EXPAND_LISTS
            DEPENDS  "${target}-makedir"
        )
        file(GENERATE OUTPUT $<TARGET_FILE_DIR:${target}>/${FILENAME}/test_config.txt CONTENT "$<TARGET_FILE:pepp-term>\n${CMAKE_CURRENT_BINARY_DIR}/pythonpath.txt")
        add_dependencies(${target} pepp-term "${target}-copyfiles")
    endfunction()

    if (NOT IOS)
        configure_cpp_pytest(test-term-about test/about.py)
        configure_cpp_pytest(test-term-asmrun test/asmrun.py)
        configure_cpp_pytest(test-term-get test/get.py)
        configure_cpp_pytest(test-term-ls test/ls.py )

        configure_cpp_pyest_sample(test-term-0625-printf test/samples/0625_printf)
        configure_cpp_pyest_sample(test-term-0627-printf test/samples/0627_printf)
        configure_cpp_pyest_sample(test-term-0629-printf test/samples/0629_printf)
        configure_cpp_pyest_sample(test-term-0636-printf test/samples/0636_printf)
        configure_cpp_pyest_sample(test-term-0638-printf test/samples/0638_printf)
        configure_cpp_pyest_sample(test-term-0642-printf test/samples/0642_printf)
        configure_cpp_pyest_sample(test-term-0644-printf test/samples/0644_printf)
        configure_cpp_pyest_sample(test-term-0646-printf test/samples/0646_printf)
        configure_cpp_pyest_sample(test-term-0648-printf test/samples/0648_printf)
        # This test has a input bug.
        # configure_cpp_pyest_sample(test-term-longstr test/samples/long_strings_error)

        #set_target_properties(test-term-0648-printf PROPERTIES FOLDER "qtc_runnable")

    endif ()
endif ()


