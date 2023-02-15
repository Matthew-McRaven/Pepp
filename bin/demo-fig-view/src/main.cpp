#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <QLocale>
#include <QQuickItem>
#include <QTranslator>
#include <qvariant.h>

//#include <QDirIterator>
#include <QQmlContext>

#include "builtins/book_item_model.hpp"
#include "builtins/registry.hpp"
int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  // TODO: Translator paths are likely broken
  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for (const QString &locale : uiLanguages) {
    const QString baseName = "figview_" + QLocale(locale).name();
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
  auto registry = QSharedPointer<builtins::Registry>::create(nullptr);
  auto model = builtins::BookModel(registry);
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("global_model",
                                           QVariant::fromValue(&model));
  const QUrl url(u"qrc:/figview/src/main.qml"_qs);
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
          QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);
  engine.load(url);
  for (const auto &object : engine.rootObjects()) {
    auto maybeChild = object->findChild<QObject *>(QStringLiteral("treeView"));
    if (auto casted = qobject_cast<QQuickItem *>(maybeChild);
        maybeChild && casted)
      casted->setProperty(
          "model",
          QVariant::fromValue(&model)); // https://stackoverflow.com/a/43742398
  }

  return app.exec();
}
