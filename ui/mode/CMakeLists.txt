file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")

# GLOB_RECURSE uses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
file(GLOB_RECURSE abs_qml_sources CONFIGURE_DEPENDS "main.qml" "*.qml")
SET(rel_qml_sources, "")
foreach (f_abs IN LISTS abs_qml_sources)
    file(RELATIVE_PATH f_rel "${CMAKE_CURRENT_LIST_DIR}" ${f_abs})
    list(APPEND rel_qml_sources "${f_rel}")
endforeach ()

qt_add_qml_module(mode
        URI "mode"
        VERSION 0.1
        QML_FILES
        ${rel_qml_sources}
        RESOURCE_PREFIX /ui
        SOURCES
        ${sources}
)

target_compile_definitions(mode PRIVATE "MODE_LIBRARY")
target_link_libraries(mode PUBLIC Qt6::Core Qt6::Gui Qt6::Quick ui_utils text memory cpu)
