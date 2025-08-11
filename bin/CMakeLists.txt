file(GLOB_RECURSE sources_and_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB_RECURSE tests CONFIGURE_DEPENDS "test/*.cpp" "test/*.hpp")
list(APPEND sources ${sources_and_tests})
list(REMOVE_ITEM sources ${tests})
# Set icon for the application if it exists.
if (EXISTS "${PROJECT_DATA_DIR}/app_icon/icon.ico" AND WIN32)
    LIST(APPEND sources "${PROJECT_DATA_DIR}/app_icon/res.rc")
elseif (EXISTS "${PROJECT_DATA_DIR}/app_icon/icon.icns" AND APPLE)
    # Name of icns file, not including directory path or extension.
    set(MACOSX_BUNDLE_ICON_FILE "icon")
    set(app_icon_macos "${PROJECT_DATA_DIR}/app_icon/icon.icns")
    set_source_files_properties(${app_icon_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    LIST(APPEND sources ${app_icon_macos})
else ()
    # Future platform-specific logic for icons.
endif ()

qt6_add_executable(pepp ${sources} ${guis}
    ${CMAKE_SOURCE_DIR}/data/icons/icons.qrc
    ${CMAKE_SOURCE_DIR}/data/app_icon/app_icon.qrc
)

# Include additional help images
file(GLOB_RECURSE help_image_files CONFIGURE_DEPENDS "${PROJECT_DATA_DIR}/help_images/macos/*.png")
qt_add_resources(pepp "help_images"
        PREFIX "/help_images"
        BASE "${PROJECT_DATA_DIR}/help_images/macos"
        FILES ${help_image_files}
)



target_link_libraries(pepp PRIVATE cli catch Qt6::Core Qt6::Gui
    Qt::Qml Qt::Quick  Qt::QuickControls2 Qt6::Svg Qt6::Xml Qt::Widgets
    test-lib-all pepp-lib pepp-libplugin KDAB::kddockwidgets kddockwidgetsplugin
)
qt_import_qml_plugins(pepp)
qt_import_plugins(pepp INCLUDE pepp-libplugin kddockwidgetsplugin INCLUDE_BY_TYPE imageformats)
target_compile_definitions(pepp PRIVATE "INCLUDE_GUI=1")
# Must occur before install scripts, otherwise MacOS builds will fail.
set_target_properties(pepp PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER edu.pepperdine.cslab.pepp
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/Info.plist
        WIN32_EXECUTABLE TRUE
        FOLDER "qtc_runnable"
)

# On non-Windows platforms, create extra files that will explicitly start the graphical or the terminal version.
# Meaningless on WIN32, since apps are either console or non-console, never both.
# EMSDK also doesn't let us execute directly, so symlinks are pointless.
if (NOT (WIN32 OR EMSCRIPTEN))
    add_custom_command(TARGET pepp POST_BUILD
            DEPENDS pepp
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            $<TARGET_FILE_BASE_NAME:pepp>
            $<LIST:JOIN,$<LIST:APPEND,$<TARGET_FILE:pepp>,-term>,>
            COMMAND chmod +x $<LIST:JOIN,$<LIST:APPEND,$<TARGET_FILE:pepp>,-term>,>
            COMMENT "Create terminal target"
    )
    # Create a helper script which will launch the main executable in GUI mode.
    file(GENERATE
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pepp-gui
            CONTENT
            "script_name=$0\n script_full_path=$(dirname \"\$0\")\n $script_full_path/$<TARGET_FILE_NAME:pepp> gui"
    )
endif ()

# Skip term target on platforms where it is not needed.
if(NOT (EMSCRIPTEN OR IOS) )
    qt6_add_executable(pepp-term ${sources}
        ${CMAKE_SOURCE_DIR}/data/icons/icons.qrc
        ${CMAKE_SOURCE_DIR}/data/app_icon/app_icon.qrc
    )
    target_link_libraries(pepp-term PRIVATE cli Qt6::Core catch test-lib-all pepp-lib catch)
    set_target_properties(pepp-term PROPERTIES FOLDER "qtc_runnable")
    target_compile_definitions(pepp-term PRIVATE "INCLUDE_GUI=0")
    target_compile_definitions(pepp-term PRIVATE "DEFAULT_GUI=0")
endif()

if (WIN32)
    # On Windows, applications may either be GUI or terminal, not both.
    # So, we (hackishly) recompile the GUI target, but this time we will not set WIN32_EXECUTABLE.
    target_compile_definitions(pepp PRIVATE "DEFAULT_GUI=1")
    # Non-DLL platforms have empty generator expression, so use more generator magic to only copy DLLs if they exist.
    add_custom_command(TARGET pepp POST_BUILD
            COMMAND ${CMAKE_COMMAND}
            ARGS
            -E $<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:pepp>>,copy,true> $<TARGET_RUNTIME_DLLS:pepp> $<TARGET_FILE_DIR:pepp>
            COMMAND_EXPAND_LISTS
    )
    # Must specify antlr, because we can't easily install 3rd-party deps.
    install(TARGETS pepp antlr4_shared pepp-term
            COMPONENT Pepp
            RUNTIME DESTINATION bin
            BUNDLE DESTINATION .
    )

elseif (APPLE)
    # Install our pepp application
    # antlr4 isn't copied by any target since it is a 3rd-party dependency.
    # we must copy it here in a platform-dependent way.
    install(TARGETS pepp antlr4_shared
            COMPONENT Pepp
            LIBRARY DESTINATION pepp.app/Contents/Frameworks
            BUNDLE DESTINATION .
    )
    set_target_properties(pepp PROPERTIES OUTPUT_NAME "Pepp")
    install(
            FILES ${CMAKE_CURRENT_BINARY_DIR}/pepp-gui
            DESTINATION Pepp.app/Contents/MacOS
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    target_compile_definitions(pepp PRIVATE "DEFAULT_GUI=1")
elseif(EMSCRIPTEN)
    # Otherwise use default install logic on Linux for now.
    install(TARGETS pepp antlr4_shared
            COMPONENT Pepp
            RUNTIME DESTINATION bin
            BUNDLE DESTINATION .
    )
    target_compile_definitions(pepp PRIVATE "DEFAULT_GUI=1")
else ()
    # Otherwise use default install logic on Linux for now.
    install(TARGETS pepp antlr4_shared
            COMPONENT Pepp
            RUNTIME DESTINATION bin
            BUNDLE DESTINATION .
    )
    target_compile_definitions(pepp PRIVATE "DEFAULT_GUI=0")
endif ()


option(ENABLE_TERM_TESTS "Enable shell tests of pepp" ON)
if(ENABLE_TERM_TESTS AND NOT (EMSCRIPTEN OR IOS))
    # Needed to find config file.
    qt6_add_executable(test-term test/about.cpp test/asmrun.cpp test/ls.cpp test/get.cpp test/test_main.cpp test/samples.qrc test/samples.cpp)
    target_include_directories(test-term PRIVATE ${CMAKE_CURRENT_LIST_DIR}/test)
    target_link_libraries(test-term PRIVATE cli catch Qt6::Core)
    add_dependencies(test-term pepp-lib pepp-libplugin pepp)
    add_test(NAME test-term COMMAND test-term)
endif ()

# Add main GUI module, qml sources, resources, and add demo applications.
# GLOB_RECURSE uses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
file(GLOB_RECURSE abs_qml_sources CONFIGURE_DEPENDS "src/*.qml")
SET(rel_qml_sources, "")
foreach (f_abs IN LISTS abs_qml_sources)
    file(RELATIVE_PATH f_rel "${CMAKE_CURRENT_LIST_DIR}" ${f_abs})
    list(APPEND rel_qml_sources "${f_rel}")
endforeach ()
qt_add_qml_module(pepp
        URI Pepp
        VERSION 0.2
        QML_FILES ${rel_qml_sources}
        SOURCES ${guis} ${demos}
        # Must be set since we overrode OUTPUT_DIRECTORY in library
        IMPORT_PATH "${CMAKE_BINARY_DIR}/edu/pepp"

)
qt6_add_resources(pepp "pepp"
        PREFIX "/"
        FILES ${pep10_resource_files}
)

# Set pepp/pepp-term as a test target.
if (NOT (EMSCRIPTEN OR IOS))
    set(count 2)
    catch_test_count(count)
    catch_add_sharded_tests(pepp-term ADDTL_ARGS "selftest" SHARD_COUNT ${count})
endif ()

qt_generate_deploy_qml_app_script(
        TARGET pepp
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script} COMPONENT Pepp)
