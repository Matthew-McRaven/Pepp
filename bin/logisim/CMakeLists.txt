cmake_minimum_required(VERSION 3.16)

qt_add_executable(
  CircuitDesign main.cpp # ui/diagramModel.cpp
  # ui/diagramModel.hpp
)

qt_add_qml_module(
  CircuitDesign
  URI
  CircuitDesign
  VERSION
  1.0
  RESOURCES
  QML_FILES
  Main.qml
  SOURCES
  ui/renderer.cpp
  DEPENDENCIES
  QtQuick)

# must include any subdirectories for classes that use QML_ELEMENT
target_include_directories(CircuitDesign PRIVATE ui/)

target_link_libraries(CircuitDesign PRIVATE Qt6::Core Qt6::Gui Qt6::Qml
                                            Qt6::Quick pepp-core)
set_target_properties(CircuitDesign PROPERTIES FOLDER "qtc_runnable")
