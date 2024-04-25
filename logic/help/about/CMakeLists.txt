cmake_minimum_required(VERSION 3.10)

file(GLOB_RECURSE sources_and_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
file(GLOB_RECURSE tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/*.cpp")
list(APPEND sources ${sources_and_tests})
list(REMOVE_ITEM sources ${tests})

make_library(
        TARGET about
        SOURCES ${sources}
        DEPENDS Qt6::Core
        TESTS ${tests}
        TEST_DEPENDS catch catch-main
)

set_target_properties(about PROPERTIES NEEDS_HTML_HELP ON)

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

#add_definitions("-DGIT_SHA1=${GIT_SHA1}")


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/version.cpp" @ONLY)

# Include dependency licenses without having to hard-code.
file(GLOB_RECURSE dependency_licenses CONFIGURE_DEPENDS "${PROJECT_DATA_DIR}/about/dep/*")

qt_add_resources(about "about"
        PREFIX "/about"
        BASE "${PROJECT_DATA_DIR}/about"
        FILES
        ${PROJECT_DATA_DIR}/about/contributors.csv ${PROJECT_DATA_DIR}/about/maintainers.csv
        ${PROJECT_DATA_DIR}/about/repo.url
        ${PROJECT_DATA_DIR}/about/dependencies.csv
        ${dependency_licenses}
)

target_sources(about PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/version.cpp")
target_sources(about PRIVATE "${CMAKE_SOURCE_DIR}/license.qrc")

