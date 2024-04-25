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
#include <QTimer>
//  Testing only
#include <QDirIterator>

#include "cpu/registermodel.h"
#include "cpu/statusbitmodel.h"
#include "help/about/version.hpp"
#include "memory/hexdump/memorybytemodel.hpp"

#include "about/registration.hpp"
#include "cpu/registration.hpp"
#include "help/registration.hpp"
#include "memory/registration.hpp"
#include "preferences/preferencemodel.hpp"
#include "preferences/registration.hpp"
#include "project/registration.hpp"
#include "text/registration.hpp"
#include "utils/registration.hpp"

struct default_data : public gui_globals {
  default_data() : pm(&theme) {}
  ~default_data() override = default;
  StatusBitModel 	sbm;
  RegisterModel 	rm;
  MemoryByteModel 	mbm;
  Theme                 theme;
  PreferenceModel 	pm;
  QTimer interval;
};

QSharedPointer<gui_globals> default_init(QQmlApplicationEngine &engine) {
  utils::registerTypes("edu.pepp");
  prefs::registerTypes("edu.pepp");
  about::registerTypes("edu.pepp");
  memory::registerTypes("edu.pepp");
  text::registerTypes("edu.pepp");
  cpu::registerTypes("edu.pepp");
  project::registerTypes("edu.pepp");
  help::registerTypes("edu.pepp");

  auto data = QSharedPointer<default_data>::create();

  //  Connect models
  auto *ctx = engine.rootContext();
  ctx->setContextProperty("StatusBitModel", 	&data->sbm);
  ctx->setContextProperty("RegisterModel", 	&data->rm);
  ctx->setContextProperty("MemoryByteModel", 	&data->mbm);
  ctx->setContextProperty("PreferenceModel", 	&data->pm);
  ctx->setContextProperty("Theme",              &data->theme);

  //  Simulate changes in Pepp10
  data->interval.setInterval(1000);
  QObject::connect(&data->interval, &QTimer::timeout, &data->sbm, &StatusBitModel::updateTestData);
  QObject::connect(&data->interval, &QTimer::timeout, &data->rm, &RegisterModel::updateTestData);
  data->interval.start();
  return data;
}

int gui_main(const gui_args &args) {
  // Must forward args for things like QML debugger to work.
  int argc = args.argvs.size();
  std::vector<char *> argvs(argc);
  // Must make copy of strings, since argvs should be editable.
  std::vector<std::string> arg_strs = args.argvs;
  for (int it = 0; it < argc; it++)
    argvs[it] = arg_strs[it].data();
  QApplication app(argc, argvs.data());

  QApplication::setOrganizationName("Pepperdine University");
  QApplication::setApplicationName("Pep/10");
  QApplication::setOrganizationDomain("pep.pepperdine.edu");
  QApplication::setApplicationDisplayName("Pep/10 IDE");
  static auto version =
      u"%1.%2.%3"_qs.arg(about::g_MAJOR_VERSION()).arg(about::g_MINOR_VERSION()).arg(about::g_PATCH_VERSION());
  QApplication::setApplicationVersion(version);

  QQuickStyle::setStyle("Fusion");

  //  Instantiate QML engine before models
  QQmlApplicationEngine engine;
  QSharedPointer<gui_globals> globals;
  if (args.extra_init)
    globals = args.extra_init(engine);
  else
    globals = default_init(engine);
  (void)globals; // Unused, but keeps bound context variables from being deleted.

  /*QDirIterator i(":", QDirIterator::Subdirectories);
  while (i.hasNext()) {
    auto f = QFileInfo(i.next());
    if (!f.isFile())
      continue;
    qDebug() << f.filePath();
  }*/

  auto ctx = engine.rootContext();

  static const auto default_entry = u"qrc:/qt/qml/Pepp/gui/main.qml"_qs;
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
