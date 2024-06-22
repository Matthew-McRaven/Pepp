file(GLOB_RECURSE core_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cxx" "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

add_library(scintilla_core STATIC ${core_sources})
target_include_directories(scintilla_core PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src" "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(scintilla_core PUBLIC lexilla)

file(GLOB_RECURSE scintilla_unit_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/unit/*.cxx")
add_executable(scintilla_unit ${scintilla_unit_sources})
target_compile_definitions(scintilla_unit PRIVATE SCINTILLA_STATIC)
set_target_properties(scintilla_unit PROPERTIES FOLDER "qtc_runnable")
target_link_libraries(scintilla_unit PRIVATE scintilla_core scintilla_catch)
add_test(NAME scintilla_unit COMMAND scintilla_unit WORKING_DIRECTORY test)
target_include_directories(scintilla_unit PUBLIC "${PROJECT_SOURCE_DIR}/catch")
