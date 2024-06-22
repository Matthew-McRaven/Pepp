file(GLOB_RECURSE core_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cxx" "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

add_library(scintilla_core STATIC ${core_sources})
target_include_directories(scintilla_core PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src" "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(scintilla_core PUBLIC lexilla)
