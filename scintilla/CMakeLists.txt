file(GLOB_RECURSE core_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cxx" "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

add_library(scintilla_core SHARED ${core_sources}
  qt/ScintillaEditBase/ScintillaText.qml)
target_include_directories(scintilla_core PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src" "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(scintilla_core PUBLIC lexilla)

file(GLOB_RECURSE scintilla_unit_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/unit/*.cxx")
add_executable(scintilla_unit ${scintilla_unit_sources})
set_target_properties(scintilla_unit PROPERTIES FOLDER "qtc_runnable")
target_link_libraries(scintilla_unit PRIVATE scintilla_core scintilla_catch)
add_test(NAME scintilla_unit COMMAND scintilla_unit WORKING_DIRECTORY test)
target_include_directories(scintilla_unit PUBLIC "${PROJECT_SOURCE_DIR}/catch")

find_package(Qt6 6.4 COMPONENTS REQUIRED Quick Core Gui Qml Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE qt_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/qt/*.cpp")
qt_add_qml_module(scintilla_qt
    URI edu.pepp
    VERSION 5.5.0
    SOURCES
        ${qt_sources}
    QML_FILES
        qt/ScintillaEditBase/ScintillaText.qml
)
target_include_directories(scintilla_qt PUBLIC "${CMAKE_CURRENT_LIST_DIR}/qt")
target_link_libraries(scintilla_qt PUBLIC scintilla_core Qt6::Quick Qt6::Core Qt6::Gui Qt6::Qml Qt6::Widgets)
target_compile_definitions(scintilla_qt PRIVATE MAKING_LIBRARY=1 SCINTILLA_QT_QML=1)
