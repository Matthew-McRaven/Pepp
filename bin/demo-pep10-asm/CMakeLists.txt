make_exec(demo-pep10-asm "./")

# GLOB_RECURSE ueses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
file(GLOB_RECURSE abs_qml_sources CONFIGURE_DEPENDS "src/**/*.qml" "src/*.qml")
SET(rel_qml_sources, "")
foreach (f_abs IN LISTS abs_qml_sources)
    file(RELATIVE_PATH f_rel "${CMAKE_CURRENT_LIST_DIR}" ${f_abs})
    list(APPEND rel_qml_sources "${f_rel}")
endforeach ()

qt_policy(SET QTP0001 NEW)

qt_add_qml_module(demo-pep10-asm
        URI pep10asm
        VERSION 1.0
        QML_FILES ${rel_qml_sources}
        )

set_target_properties(demo-pep10-asm PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
        )
set_target_properties(demo-pep10-asm PROPERTIES FOLDER "qtc_runnable")

target_link_libraries(demo-pep10-asm
        PRIVATE Qt6::Quick macro pas builtins Qt6::Widgets)

install(TARGETS demo-pep10-asm
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
