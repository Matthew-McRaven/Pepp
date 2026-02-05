cmake_minimum_required(VERSION 3.16)

project(CircuitDesign VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

set(CMAKE_AUTORCC ON)

qt_add_executable(CircuitDesign
    main.cpp
    svg/CircuitDesign.qrc
    #ui/diagramModel.cpp ui/diagramModel.hpp
)

qt_add_qml_module(CircuitDesign
    URI CircuitDesign
    VERSION 1.0
    #URI com.pepperdine.edu
    RESOURCES svg/CircuitDesign.qrc
    QML_FILES
        Main.qml
        ui/LogicGates.qml
        ui/Diagram.qml
        ui/DiagramCanvas.qml
        ui/DiagramEditor.qml
        ui/DiagramEndpoint.qml
        ui/Line.qml
        ui/GridLines.qml
        ui/DiagramListView.qml
        #ui/DiagramTableViewDelegate.qml
    SOURCES
        ui/diagramdata.hpp
        ui/diagramdata.cpp
        ui/diagramdatamodel.hpp
        ui/diagramdatamodel.cpp
        ui/diagramkey.hpp
        ui/diagramtype.hpp
        ui/diagramlistmodel.hpp
        ui/diagramlistmodel.cpp
        ui/diagramproperty.hpp
        ui/diagramproperty.cpp
        ui/graphiccanvas.h
        ui/graphiccanvas.cpp
    DEPENDENCIES QtQuick
)

# must include any subdirectories for classes that use QML_ELEMENT
target_include_directories(${PROJECT_NAME} PRIVATE
      ui/
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(CircuitDesign PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appCircuitDesign
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(CircuitDesign PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS CircuitDesign
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
