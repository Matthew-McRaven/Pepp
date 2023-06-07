cmake_minimum_required(VERSION 3.10)

make_library(about about)
target_include_directories(about PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(about PUBLIC "Qt6::Core;${Boost_LIBRARIES}")
#target_compile_definitions(about PRIVATE ABOUT_LIBRARY)

include(git.cmake)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
#add_definitions("-DGIT_SHA1=${GIT_SHA1}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/about/version.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/version.cpp" @ONLY)
qt_add_resources(about "about"
  PREFIX "/about"
  BASE "data"
  FILES
    data/LICENSE_FULL
    data/contributors.csv data/maintainers.csv
)
set_source_files_properties(data/LICENSE_FULL QT_RESOURCE_ALIAS LICENSE)
target_sources(about PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/version.cpp")
