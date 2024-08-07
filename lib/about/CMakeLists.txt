cmake_minimum_required(VERSION 3.10)

file(GLOB_RECURSE sources_and_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
list(APPEND sources ${sources_and_tests})

# GLOB_RECURSE uses abs paths, QML_FILES expects relative paths. This blob converts from abs to rel.
file(GLOB_RECURSE abs_qml_sources CONFIGURE_DEPENDS "*.qml")
SET(rel_qml_sources, "")
foreach (f_abs IN LISTS abs_qml_sources)
    file(RELATIVE_PATH f_rel "${CMAKE_CURRENT_LIST_DIR}" ${f_abs})
    list(APPEND rel_qml_sources "${f_rel}")
endforeach ()


include(git.cmake)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_get_exact_tag(GIT_TAG)
if (GIT_TAG MATCHES ".*NOTFOUND")
    set(GIT_TAG "unknown")
endif ()
# Let program know if it was built from a clean dir.
# App can use that to render about info.
git_local_changes(GIT_CLEAN_RAW)
if (GIT_CLEAN_RAW MATCHES "CLEAN")
    set(GIT_CLEAN "1")
else ()
    set(GIT_CLEAN "0")
endif ()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version_gen.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/version_gen.cpp" @ONLY)

qt_add_qml_module(about
        URI "about"
        STATIC
        SOURCES ${sources} "${CMAKE_SOURCE_DIR}/resource_license.qrc" "${CMAKE_CURRENT_BINARY_DIR}/version_gen.cpp"
        QML_FILES ${rel_qml_sources}
        RESOURCE_PREFIX /ui
        NO_PLUGIN
)

target_link_libraries(about PUBLIC Qt6::Core Qt6::Gui Qt6::Quick)
set_target_properties(about PROPERTIES NEEDS_HTML_HELP ON)


# Include dependency licenses without having to hard-code.
file(GLOB_RECURSE dependency_licenses CONFIGURE_DEPENDS "${PROJECT_DATA_DIR}/about/dep/*")

set_source_files_properties(${CMAKE_SOURCE_DIR}/NOTICE PROPERTIES QT_RESOURCE_ALIAS LICENSE_NOTICE)
set_source_files_properties(${CMAKE_SOURCE_DIR}/LICENSE PROPERTIES QT_RESOURCE_ALIAS LICENSE_FULL)

qt_add_resources(about "about"
        PREFIX "/about"
        BASE "${PROJECT_DATA_DIR}/about"
        FILES
        ${PROJECT_DATA_DIR}/about/contributors.csv ${PROJECT_DATA_DIR}/about/maintainers.csv
        ${PROJECT_DATA_DIR}/about/repo.url
        ${PROJECT_DATA_DIR}/about/dependencies.csv
        ${dependency_licenses}
        ${CMAKE_SOURCE_DIR}/NOTICE
        ${CMAKE_SOURCE_DIR}/LICENSE
)

