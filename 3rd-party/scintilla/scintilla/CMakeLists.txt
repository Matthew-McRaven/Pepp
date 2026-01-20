file(GLOB_RECURSE core_sources CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_LIST_DIR}/src/*.cxx"
     "${CMAKE_CURRENT_LIST_DIR}/include/*.h")
file(GLOB_RECURSE qt_sources CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_LIST_DIR}/qt/*.cpp")

set(WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

qt_add_qml_module(
  scintilla_qt
  URI
  "scintilla"
  STATIC
  VERSION
  5.5.0
  SOURCES
  ${qt_sources}
  ${core_sources}
  RESOURCE_PREFIX
  /ui)
target_include_directories(
  scintilla_qt
  PUBLIC "${CMAKE_CURRENT_LIST_DIR}/qt" "${CMAKE_CURRENT_LIST_DIR}/qt"
         "${CMAKE_CURRENT_LIST_DIR}/src" "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(scintilla_qt PUBLIC lexilla Qt6::Quick Qt6::Core Qt6::Gui
                                          Qt6::Qml Qt6::Widgets)
target_compile_definitions(scintilla_qt PUBLIC SCINTILLA_STATIC)

if(NOT IOS)
  file(GLOB_RECURSE scintilla_unit_sources CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_LIST_DIR}/test/unit/*.cxx")
  add_executable(scintilla_unit_qt ${scintilla_unit_sources})
  target_link_libraries(scintilla_unit_qt PRIVATE scintilla_qt catch catch-main)
  add_test(
    NAME scintilla_unit_qt
    COMMAND scintilla_unit_qt
    WORKING_DIRECTORY test)
  target_include_directories(scintilla_unit_qt PUBLIC)
endif()
