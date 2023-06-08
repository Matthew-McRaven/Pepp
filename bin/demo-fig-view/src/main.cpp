/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
