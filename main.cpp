#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>

#include "diagramproperty.hpp"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterUncreatableType<DiagramProperty>("DiagramEnum",
                                                1,
                                                0,
                                                "DiagramProperty",
                                                "Cannot be instantiated, only used for enums");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("CircuitDesign", "Main");

    app.setWindowIcon(QIcon{":/icon"});

    return app.exec();
}

/*  To Do
 *  
 *  Add toolbar with selection modes
 *  Filter sidebar to only show stencils available from toolbar
 *  Display shadow where object will drop using hover. 
 */
