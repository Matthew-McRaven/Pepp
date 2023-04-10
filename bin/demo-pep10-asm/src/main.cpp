#include <QGuiApplication>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QTranslator>
#include <qvariant.h>

#include <QComboBox>
#include <qstringlistmodel.h>

#include "assembly.hpp"
#include "figure.hpp"
int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  // TODO: Translator paths are likely broken
  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for (const QString &locale : uiLanguages) {
    const QString baseName = "pep10asm_" + QLocale(locale).name();
    if (translator.load(":/i18n/" + baseName)) {
      app.installTranslator(&translator);
      break;
    }
  }
  /*QDirIterator i(":", QDirIterator::Subdirectories);
  while (i.hasNext()) {
    auto f = QFileInfo(i.next());
    if (!f.isFile())
      continue;
    qDebug() << f.filePath();
  }*/
  QQmlApplicationEngine engine;
  qmlRegisterType<FigureManager>("edu.pepperdine.cslab.p10asm", 1, 0,
                                 "FigureManager");
  qmlRegisterType<builtins::Figure>("edu.pepperdine.cslab.p10asm", 1, 0,
                                    "Figure");
  qmlRegisterType<AssemblyManger>("edu.pepperdine.cslab.p10asm", 1, 0,
                                  "AssemblyManager");
  /*qmlRegisterType<MacroParser>("edu.pepperdine.cslab.macroparse", 1, 0,
                               "MacroParser");*/
  const QUrl url(u"qrc:/pep10asm/src/main.qml"_qs);
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
          QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);
  engine.load(url);
  return app.exec();
}
