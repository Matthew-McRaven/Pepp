file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/src/*.hpp")

make_target(
  TARGET demo-pep10-asm
  TYPE "EXEC"
  SOURCES ${sources}
  DEPENDS Qt6::Core Qt6::Quick macro pas builtins Qt6::Widgets
)

# GLOB_RECURSE ueses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
file(GLOB_RECURSE abs_qml_sources CONFIGURE_DEPENDS "src/**/*.qml" "src/*.qml")
SET(rel_qml_sources, "")
foreach (f_abs IN LISTS abs_qml_sources)
    file(RELATIVE_PATH f_rel "${CMAKE_CURRENT_LIST_DIR}" ${f_abs})
    list(APPEND rel_qml_sources "${f_rel}")
endforeach ()

qt6_policy(SET QTP0001 NEW)

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
set_target_properties(demo-pep10-asm PROPERTIES EMBED_GH_PAGES ON)
