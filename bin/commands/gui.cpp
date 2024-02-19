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
#include "gui.hpp"
#if INCLUDE_GUI
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
//  Testing only
#include <QDirIterator>
#include <QTimer>

#include "../gui/cpu/registermodel.h"
#include "../gui/cpu/statusbitmodel.h"
#include "memory/hexdump/memorybytemodel.h"
struct default_data : public gui_globals {
  ~default_data() override = default;
  StatusBitModel sbm;
  RegisterModel rm;
  MemoryByteModel mbm;
  QTimer interval;
};

QSharedPointer<gui_globals> default_init(QQmlApplicationEngine &engine) {
  // Register the type DataEntryModel
  // under the url "edu.pepperdine" in version 1.0

  //  This method allows the model to be instantiated from QML.
  //  This is only good if we are not expecting events from C++
  // qmlRegisterType<StatusBitModel>("edu.pepperdine", 1, 0, "StatusBitModel");

  //  Instantiate models
  //  Note, these models are instantiated in C++ and passed to QML. QML
  //  cannot instantiate these models directly
  qmlRegisterUncreatableType<MemoryByteModel>("edu.pepperdine", 1, 0, "MemByteRoles", "Error: only enums");
  // qRegisterMetaType<MemoryColumns>();
  auto data = QSharedPointer<default_data>::create();

  //  Connect models
  auto *ctx = engine.rootContext();
  ctx->setContextProperty("StatusBitModel", &data->sbm);
  ctx->setContextProperty("RegisterModel", &data->rm);
  ctx->setContextProperty("MemoryByteModel", &data->mbm);

  //  Simulate changes in Pepp10
  data->interval.setInterval(1000);
  QObject::connect(&data->interval, &QTimer::timeout, &data->sbm, &StatusBitModel::updateTestData);
  QObject::connect(&data->interval, &QTimer::timeout, &data->rm, &RegisterModel::updateTestData);
  QObject::connect(&data->interval, &QTimer::timeout, &data->mbm, &MemoryByteModel::updateTestData);
  data->interval.start();
  return data;
}

int gui_main(const gui_args &args) {
  int argc = 0;
  char **argv = nullptr;
  QApplication app(argc, argv);

  QApplication::setOrganizationName("Pepperdine University");
  QApplication::setApplicationName("Pep/10");
  QApplication::setOrganizationDomain("pep.pepperdine.edu");
  QApplication::setApplicationDisplayName("Pep/10 IDE");
  QApplication::setApplicationVersion("0.0.0.3");

  QQuickStyle::setStyle("Fusion");

  //  Instantiate QML engine before models
  QQmlApplicationEngine engine;
  QSharedPointer<gui_globals> globals;
  if (args.extra_init)
    globals = args.extra_init(engine);
  else
    globals = default_init(engine);
  (void)globals; // Unused, but keeps bound context variables from being deleted.

  QDirIterator i(":", QDirIterator::Subdirectories);
  while (i.hasNext()) {
    auto f = QFileInfo(i.next());
    if (!f.isFile())
      continue;
    qDebug() << f.filePath();
  }

  static const auto default_entry = u"qrc:/Pepp/gui/main.qml"_qs;
  const QUrl url(args.QMLEntry.isEmpty() ? default_entry : args.QMLEntry);

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

#else
int gui_main(gui_args) { return 0; }
#endif
