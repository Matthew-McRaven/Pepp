qt_add_executable(CircuitDesign svg/CircuitDesign.qrc main.cpp)

qt_add_qml_module(
  CircuitDesign
  URI
  CircuitDesign
  VERSION
  1.0
  RESOURCES
  svg/CircuitDesign.qrc
  QML_FILES
  Main.qml
  ui/LogicGates.qml
  ui/DiagramCanvas.qml
  ui/ComponentEditor.qml
  ui/BluePrintListView.qml
  SOURCES

  ui/blueprintlibrarymodel.hpp
  ui/blueprintlibrarymodel.cpp
  ui/componentpropertymodel.hpp
  ui/componentpropertymodel.cpp
  ui/diagramproperty.hpp
  ui/diagramproperty.cpp
  ui/graphiccanvas.hpp
  ui/graphiccanvas.cpp
  ui/pixmaps/mipmappixmap.cpp
  ui/pixmaps/mipmapsource.cpp
  ui/pixmaps/mipmapstore.cpp
  ui/pixmaps/prerotatedpixmap.cpp
  ui/schematic/component.cpp
  ui/schematic/blueprint.cpp
  ui/schematic/net.cpp
  ui/schematic/blueprintlibrary.cpp
  ui/schematic/circuitschematic.cpp
  ui/schematic/orient.cpp
  ui/schematic/circuitproject.cpp
  DEPENDENCIES
  QtQuick)

# must include any subdirectories for classes that use QML_ELEMENT
target_include_directories(CircuitDesign PRIVATE ui/)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1. If
# you are developing for iOS or macOS you should consider setting an explicit,
# fixed bundle identifier manually though.
set_target_properties(
  CircuitDesign
  PROPERTIES # MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appCircuitDesign
             MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
             MACOSX_BUNDLE TRUE
             WIN32_EXECUTABLE TRUE)

target_link_libraries(
  CircuitDesign
  PRIVATE Qt6::Core
          Qt6::Gui
          Qt6::Svg
          Qt6::Qml
          Qt6::Quick
          Qt6::Widgets
          pepp-core)

set_target_properties(CircuitDesign PROPERTIES FOLDER "qtc_runnable")
