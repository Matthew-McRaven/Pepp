cmake_minimum_required(VERSION 3.24)


file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cxx" "${CMAKE_CURRENT_LIST_DIR}/include/*.h")
file(GLOB_RECURSE lexers CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/lexers/*.cxx" )
file(GLOB_RECURSE lexlib CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/lexlib/*.cxx" "${CMAKE_CURRENT_LIST_DIR}/lexlib/*.h")
file(GLOB_RECURSE access CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/access/*.cxx" "${CMAKE_CURRENT_LIST_DIR}/access/*.h")

add_library(lexilla STATIC ${sources} ${lexers} ${lexlib} ${access})
target_compile_definitions(lexilla PRIVATE LEXILLA_STATIC)
target_include_directories(lexilla PUBLIC "${CMAKE_CURRENT_LIST_DIR}/lexlib"
 "${CMAKE_CURRENT_LIST_DIR}/include" "${CMAKE_CURRENT_LIST_DIR}/access"
 "${CMAKE_CURRENT_LIST_DIR}/../scintilla/include"
 )
