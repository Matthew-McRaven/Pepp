cmake_minimum_required(VERSION 3.10)

make_library(about about)
set_target_properties(about PROPERTIES NEEDS_HTML_HELP ON)
target_include_directories(about PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(about PUBLIC "Qt6::Core" )
#target_compile_definitions(about PRIVATE ABOUT_LIBRARY)

include(git.cmake)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

git_get_exact_tag(GIT_TAG)



if(GIT_TAG MATCHES ".*NOTFOUND")
    set(GIT_TAG "unknown")
endif()

# Let program know if it was built from a clean dir.
# App can use that to render about info.
git_local_changes(GIT_CLEAN_RAW)
if(GIT_CLEAN_RAW MATCHES "CLEAN")
    set(GIT_CLEAN "1")
else()
  set(GIT_CLEAN "0")
endif()

#add_definitions("-DGIT_SHA1=${GIT_SHA1}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/about/version.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/version.cpp" @ONLY)

# Include dependency licenses without having to hard-code.
file(GLOB_RECURSE dependency_licenses CONFIGURE_DEPENDS "data/dep/*")

qt_add_resources(about "about"
        PREFIX "/about"
        BASE "data"
        FILES
        data/LICENSE_FULL data/LICENSE_NOTICE
        data/contributors.csv data/maintainers.csv
        data/repo.url
        data/dependencies.csv
        ${dependency_licenses}
        )
set_source_files_properties("data/LICENSE_FULL" PROPERTIES QT_RESOURCE_ALIAS "LICENSE")
set_source_files_properties("data/LICENSE_NOTICE" PROPERTIES QT_RESOURCE_ALIAS "NOTICE")

target_sources(about PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/version.cpp")

if (NOT IOS)
    make_qtest(test-about-dependencies test/dependencies.cpp "about;Qt6::Core;Qt::Test")
    make_qtest(test-about-pepp test/pepp.cpp "about;Qt6::Core;Qt::Test")
    set_target_properties(test-about-pepp PROPERTIES FOLDER "qtc_runnable")
endif ()
