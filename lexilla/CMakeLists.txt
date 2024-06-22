

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

file(GLOB_RECURSE lexilla_unit_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/unit/*.cxx")
add_executable(lexilla_unit ${lexilla_unit_sources})
target_compile_definitions(lexilla_unit PRIVATE LEXILLA_STATIC)
set_target_properties(lexilla_unit PROPERTIES FOLDER "qtc_runnable")
target_link_libraries(lexilla_unit PRIVATE lexilla)
add_test(NAME lexilla_unit COMMAND lexilla_unit WORKING_DIRECTORY test)

add_executable(lexilla_test test/TestDocument.cxx test/TestLexers.cxx)
# Does not create a dependency. If examples are updated, they will not be re-copied.
file(COPY ${CMAKE_CURRENT_LIST_DIR}/test/examples DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test)
target_compile_definitions(lexilla_test PRIVATE LEXILLA_STATIC)
set_target_properties(lexilla_test PROPERTIES FOLDER "qtc_runnable")
target_link_libraries(lexilla_test PRIVATE lexilla)
add_test(NAME lexilla_test COMMAND lexilla_test WORKING_DIRECTORY test)
