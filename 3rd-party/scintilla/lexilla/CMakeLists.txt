file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cxx" "${CMAKE_CURRENT_LIST_DIR}/include/*.h")
file(GLOB_RECURSE lexers CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/lexers/*.cxx" )
file(GLOB_RECURSE lexlib CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/lexlib/*.cxx" "${CMAKE_CURRENT_LIST_DIR}/lexlib/*.h")
file(GLOB_RECURSE access CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/access/*.cxx" "${CMAKE_CURRENT_LIST_DIR}/access/*.h")

set(WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(lexilla STATIC ${sources} ${lexers} ${lexlib} ${access})
target_link_libraries(lexilla PUBLIC Qt6::Core)
target_compile_definitions(lexilla PUBLIC LEXILLA_STATIC)
target_include_directories(lexilla PUBLIC "${CMAKE_CURRENT_LIST_DIR}/lexlib"
 "${CMAKE_CURRENT_LIST_DIR}/include" "${CMAKE_CURRENT_LIST_DIR}/access" "${CMAKE_CURRENT_LIST_DIR}/../scintilla/include")

set(CMAKE_AUTORCC ON)



if(NOT IOS)
    file(GLOB_RECURSE lexilla_unit_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/unit/*.cxx")
    add_executable(lexilla_unit ${lexilla_unit_sources} )
    target_link_libraries(lexilla_unit PRIVATE lexilla scintilla_catch )
    add_test(NAME lexilla_unit COMMAND lexilla_unit WORKING_DIRECTORY test)
    target_include_directories(lexilla_unit PUBLIC "${PROJECT_SOURCE_DIR}/catch")

    qt_add_executable(lexilla_test test/TestDocument.cxx test/TestLexers.cxx)
    target_link_libraries(lexilla_test PRIVATE lexilla Qt6::Core)
    add_test(NAME lexilla_test COMMAND lexilla_test ${CMAKE_CURRENT_LIST_DIR}/test/examples WORKING_DIRECTORY test)
endif()
