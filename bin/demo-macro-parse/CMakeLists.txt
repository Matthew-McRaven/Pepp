make_exec(demo-macro-parse "./")

# GLOB_RECURSE ueses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
file(GLOB_RECURSE abs_qml_sources CONFIGURE_DEPENDS "src/**/*.qml" "src/*.qml")
SET(rel_qml_sources, "")
foreach (f_abs IN LISTS abs_qml_sources)
    file(RELATIVE_PATH f_rel "${CMAKE_CURRENT_LIST_DIR}" ${f_abs})
    list(APPEND rel_qml_sources "${f_rel}")
endforeach ()

qt6_policy(SET QTP0001 NEW)

qt_add_qml_module(demo-macro-parse
        URI macroparse
        VERSION 1.0
        QML_FILES ${rel_qml_sources}
        )

set_target_properties(demo-macro-parse PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
        )
set_target_properties(demo-macro-parse PROPERTIES FOLDER "qtc_runnable")

target_link_libraries(demo-macro-parse
        PRIVATE Qt6::Quick macro)

install(TARGETS demo-macro-parse
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
