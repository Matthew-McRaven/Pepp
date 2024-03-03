file(GLOB_RECURSE sources_and_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
file(GLOB_RECURSE old_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/highlight/__old/*.cpp")
list(APPEND sources ${sources_and_tests})
list(REMOVE_ITEM sources ${old_sources})

# GLOB_RECURSE uses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
file(GLOB_RECURSE abs_qml_sources CONFIGURE_DEPENDS "main.qml" "*.qml")
SET(rel_qml_sources, "")
foreach (f_abs IN LISTS abs_qml_sources)
    file(RELATIVE_PATH f_rel "${CMAKE_CURRENT_LIST_DIR}" ${f_abs})
    list(APPEND rel_qml_sources "${f_rel}")
endforeach ()

qt_add_qml_module(text
        URI "text"
        VERSION 0.1
        QML_FILES
        ${rel_qml_sources}
        RESOURCE_PREFIX /ui
        SOURCES
        ${sources}
)

target_compile_definitions(text PRIVATE "TEXT_LIBRARY")
target_link_libraries(text PUBLIC Qt6::Core Qt6::Gui Qt6::Quick isa)
target_include_directories(text PRIVATE ${CMAKE_CURRENT_LIST_DIR}/editor)
