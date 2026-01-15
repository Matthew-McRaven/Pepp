cmake_minimum_required(VERSION 3.16)

project(CircuitDesign VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

set(CMAKE_AUTORCC ON)

qt_add_executable(CircuitDesign
    main.cpp
    svg/CircuitDesign.qrc
)

qt_add_qml_module(CircuitDesign
    URI CircuitDesign
    QML_FILES
        Main.qml
        RESOURCES svg/CircuitDesign.qrc
        QML_FILES ui/move.js
        QML_FILES ui/LogicGates.qml
        QML_FILES ui/DragTile.qml
        QML_FILES ui/DropTile.qml
        QML_FILES ui/Diagram.qml
        QML_FILES ui/Line.qml
        QML_FILES ui/GridLine.qml
        QML_FILES ui/DiagramProperties.qml
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

target_link_libraries(CircuitDesign
    PRIVATE Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS CircuitDesign
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
