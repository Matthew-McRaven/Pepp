file(GLOB_RECURSE sources_and_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB_RECURSE tests CONFIGURE_DEPENDS "test/*.cpp" "test/*.hpp")
file(GLOB_RECURSE compile_tests CONFIGURE_DEPENDS "compile-test/*.cpp")
file(GLOB_RECURSE demos CONFIGURE_DEPENDS "demo/*.cpp")
file(GLOB_RECURSE guis CONFIGURE_DEPENDS "gui/*.cpp")
list(APPEND sources ${sources_and_tests})
list(REMOVE_ITEM sources ${tests})
list(REMOVE_ITEM sources ${compile_tests})
list(REMOVE_ITEM sources ${demos})
list(REMOVE_ITEM sources ${guis})

# Use to force terminal, test app to live in same directory.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/output")

make_target(
        TARGET pepp
        TYPE "EXEC"
        SOURCES ${sources} ${guis}
        DEPENDS
        TextFlow cli about macro pas builtins isa targets sim
        Qt6::Core Qt6::Gui Qt::Qml Qt::Quick Qt::QuickControls2
        Qt::Widgets test-lib-all memory
)


target_compile_definitions(pepp PRIVATE "INCLUDE_GUI=1")
# On Windows, applications may either be GUI or terminal, not both.
# So, we (hackishly) recompile the target above, but this time we will not set WIN32_EXECUTABLE.
if (WIN32)
    make_target(
            TARGET pepp-term
            TYPE "EXEC"
            SOURCES ${sources}
            DEPENDS TextFlow cli about macro pas builtins isa targets sim Qt6::Core catch test-lib-all
    )
    set_target_properties(pepp-term PROPERTIES FOLDER "qtc_runnable")
    target_compile_definitions(pepp-term PRIVATE "INCLUDE_GUI=0")
    target_compile_definitions(pepp-term PRIVATE "DEFAULT_GUI=0")
    target_compile_definitions(pepp PRIVATE "DEFAULT_GUI=1")
else ()
    target_compile_definitions(pepp PRIVATE "DEFAULT_GUI=0")
endif ()

# GLOB_RECURSE uses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
set_target_properties(pepp PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)

# Non-DLL platforms have empty generator expressin, so use more generator magic to only copy DLLs if the exist.
add_custom_command(TARGET pepp POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS
        -E $<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:pepp>>,copy,true> $<TARGET_RUNTIME_DLLS:pepp> $<TARGET_FILE_DIR:pepp>
        COMMAND_EXPAND_LISTS
)


set_target_properties(pepp PROPERTIES FOLDER "qtc_runnable")

if (NOT EMSCRIPTEN)
    configure_file(test/config.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/config.hpp")
    make_target(
            TARGET test-term
            TYPE "EXEC"
            SOURCES
            test/about.cpp
            test/asmrun.cpp
            test/ls.cpp
            test/get.cpp
            test/test_main.cpp
            test/samples.qrc
            test/samples.cpp
            DEPENDS Qt6::Core catch
    )

    # Needed to find config file.
    target_include_directories(test-term PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    add_dependencies(pepp test-term)
    option(ENABLE_TERM_TESTS "Enable shell tests of pepp" OFF)
    if (ENABLE_TERM_TESTS)
        add_test(NAME test-term COMMAND test-term)
        set_target_properties(test-term PROPERTIES FOLDER "qtc_runnable")
    endif ()
endif ()

# Add main GUI module, qml sources, resources, and add demo applications.
# GLOB_RECURSE uses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
file(GLOB_RECURSE abs_qml_sources CONFIGURE_DEPENDS "gui/*.qml")
SET(rel_qml_sources, "")
foreach (f_abs IN LISTS abs_qml_sources)
    file(RELATIVE_PATH f_rel "${CMAKE_CURRENT_LIST_DIR}" ${f_abs})
    list(APPEND rel_qml_sources "${f_rel}")
endforeach ()
qt_add_qml_module(pepp
        URI Pepp
        VERSION 0.2
        QML_FILES ${rel_qml_sources}
        SOURCES ${guis}
        QML_FILES
)
qt6_add_resources(pepp "pepp"
        PREFIX
        "/"
        FILES
        ${pep10_resource_files}
)