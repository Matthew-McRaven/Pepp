/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QtQml/QQmlExtensionPlugin>

//  Testing only
#include <QDirIterator>
#include <QTimer>

#include "memory/hexdump/memorybytemodel.hpp"
#include "model/registermodel.h"
#include "model/statusbitmodel.h"
#include "preferences/preferencemodel.hpp"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  QApplication::setOrganizationName("Pepperdine University");
  QApplication::setApplicationName("Pep/10");
  QApplication::setOrganizationDomain("pep.pepperdine.edu");
  QApplication::setApplicationDisplayName("Pep/10 IDE");
  QApplication::setApplicationVersion("0.0.0.3");

  QQuickStyle::setStyle("Fusion");

  //  Instantiate QML engine before models
  QQmlApplicationEngine engine;

  // Register the type DataEntryModel
  // under the url "edu.pepperdine" in version 1.0

  //  This method allows the model to be instantiated from QML.
  //  This is only good if we are not expecting events from C++
  // qmlRegisterType<StatusBitModel>("edu.pepperdine", 1, 0, "StatusBitModel");

  //  Instantiate models
  //  Note, these models are instantiated in C++ and passed to QML. QML
  //  cannot instantiate these models directly
  qmlRegisterUncreatableType<MemoryByteModel>(
      "edu.pepperdine", 1, 0, "MemByteRoles", "Error: only enums");
  // qRegisterMetaType<MemoryColumns>();

  StatusBitModel sbm;
  RegisterModel rm;
  MemoryByteModel mbm;
  PreferenceModel pm;

  //  Connect models
  auto *ctx = engine.rootContext();
  ctx->setContextProperty("StatusBitModel", &sbm);
  ctx->setContextProperty("RegisterModel", &rm);
  ctx->setContextProperty("MemoryByteModel", &mbm);
  ctx->setContextProperty("PreferenceModel", &pm);

  const QUrl url(u"qrc:/Pep10/main.qml"_qs);

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
          QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);
  /*QDirIterator i(":", QDirIterator::Subdirectories);
  while (i.hasNext()) {
      auto f = QFileInfo(i.next());
      if (!f.isFile())
          continue;
      qDebug() << f.filePath();
  }*/
  engine.load(url);

  //  Simulate changes in Pepp10
  QTimer interval;
  interval.setInterval(1000);
  QObject::connect(&interval, &QTimer::timeout, &sbm,
                   &StatusBitModel::updateTestData);
  QObject::connect(&interval, &QTimer::timeout, &rm,
                   &RegisterModel::updateTestData);
  // QObject::connect(&interval, &QTimer::timeout,
  //                  &mm, &MemoryModel::updateTestData);
  // QObject::connect(&interval, &QTimer::timeout,
  //                  &mbm, &MemoryByteModel::updateTestData);
  interval.start();

  return app.exec();

  interval.stop();
}

void timerEvent(QTimerEvent *event) { qDebug() << "Update..."; }
