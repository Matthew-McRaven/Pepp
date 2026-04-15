#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>

#include "diagramproperty.hpp"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

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
 */
