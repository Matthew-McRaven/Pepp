file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")

# GLOB_RECURSE uses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
file(GLOB_RECURSE abs_qml_sources CONFIGURE_DEPENDS "*.qml")
SET(rel_qml_sources, "")
foreach (f_abs IN LISTS abs_qml_sources)
    file(RELATIVE_PATH f_rel "${CMAKE_CURRENT_LIST_DIR}" ${f_abs})
    list(APPEND rel_qml_sources "${f_rel}")
endforeach ()


qt_add_qml_module(macro
        URI "macro"
        VERSION 0.1
        STATIC
        SOURCES ${sources}
        QML_FILES ${rel_qml_sources}
        RESOURCE_PREFIX /ui
        NO_PLUGIN
)
target_link_libraries(macro PUBLIC Qt6::Core antlr4_shared)

# Will fail to find hpp files without this, see https://bugreports.qt.io/browse/QTBUG-101146.
get_target_property(ANTLR4_INCLUDE_DIR antlr4_shared INCLUDE_DIRECTORIES)
target_include_directories(macro PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${ANTLR4_INCLUDE_DIR})
