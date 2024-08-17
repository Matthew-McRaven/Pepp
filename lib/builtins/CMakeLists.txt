cmake_minimum_required(VERSION 3.10)

file(GLOB_RECURSE cs4e-res-abs CONFIGURE_DEPENDS "${PROJECT_DATA_DIR}/books/cs4e/*")
SET(cs4e-res, "")
foreach (f-abs IN LISTS cs4e-res-abs)
    file(RELATIVE_PATH f-rel "${CMAKE_CURRENT_LIST_DIR}" ${f-abs})
    list(APPEND cs4e-res "${f-rel}")
endforeach ()

file(GLOB_RECURSE cs5e-res-abs CONFIGURE_DEPENDS "${PROJECT_DATA_DIR}/books/cs5e/*")
SET(cs5e-res, "")
foreach (f-abs IN LISTS cs5e-res-abs)
    file(RELATIVE_PATH f-rel "${CMAKE_CURRENT_LIST_DIR}" ${f-abs})
    list(APPEND cs5e-res "${f-rel}")
endforeach ()

file(GLOB_RECURSE cs6e-res-abs CONFIGURE_DEPENDS "${PROJECT_DATA_DIR}/books/cs6e/*")
SET(cs6e-res, "")
foreach (f-abs IN LISTS cs6e-res-abs)
    file(RELATIVE_PATH f-rel "${CMAKE_CURRENT_LIST_DIR}" ${f-abs})
    list(APPEND cs6e-res "${f-rel}")
endforeach ()

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")

# GLOB_RECURSE uses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
file(GLOB_RECURSE abs_qml_sources CONFIGURE_DEPENDS "*.qml")
SET(rel_qml_sources, "")
foreach (f_abs IN LISTS abs_qml_sources)
    file(RELATIVE_PATH f_rel "${CMAKE_CURRENT_LIST_DIR}" ${f_abs})
    list(APPEND rel_qml_sources "${f_rel}")
endforeach ()

qt_add_qml_module(builtins
        URI "builtins"
        VERSION 0.1
        STATIC
        SOURCES ${sources}
        QML_FILES ${rel_qml_sources}
        RESOURCE_PREFIX /ui
        NO_PLUGIN
)
target_link_libraries(builtins PUBLIC Qt6::Core Qt6::Gui Qt6::Quick macro)

qt_add_resources(builtins "builtins" PREFIX "/" BASE "${PROJECT_DATA_DIR}/" FILES ${cs4e-res} ${cs5e-res} ${cs6e-res})
