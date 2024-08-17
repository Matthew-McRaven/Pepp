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
#include <registration.hpp>
#include "../iconprovider.hpp"
#include "about/version.hpp"
#include "preferences/preferencemodel.hpp"
#include "preferences/theme.hpp"
//  Testing only
#include <QDirIterator>

struct default_data : public gui_globals {
  default_data() : pm(&theme) {}
  ~default_data() override = default;
  Theme theme;
  PreferenceModel pm;
  QTimer interval;
};

QSharedPointer<gui_globals> default_init(QQmlApplicationEngine &engine, QSharedPointer<default_data> data) {
  registerTypes("edu.pepp");

  //  Connect models
  auto *ctx = engine.rootContext();
  ctx->setContextProperty("PreferenceModel", &data->pm);
  ctx->setContextProperty("Theme", &data->theme);

  return data;
}

int gui_main(const gui_args &args) {
  // Must forward args for things like QML debugger to work.
  int argc = args.argvs.size();
  std::vector<char *> argvs(argc);
  // Must make copy of strings, since argvs should be editable.
  std::vector<std::string> arg_strs = args.argvs;
  for (int it = 0; it < argc; it++) argvs[it] = arg_strs[it].data();
  QApplication app(argc, argvs.data());

  QApplication::setOrganizationName("Pepperdine University");
  QApplication::setApplicationName("Pep/10");
  QApplication::setOrganizationDomain("pep.pepperdine.edu");
  QApplication::setApplicationDisplayName("Pep/10 IDE");
  static auto version =
      u"%1.%2.%3"_qs.arg(about::g_MAJOR_VERSION()).arg(about::g_MINOR_VERSION()).arg(about::g_PATCH_VERSION());
  QApplication::setApplicationVersion(version);
  QQuickStyle::setStyle("Fusion");

  //  Data must outlive QML engine, or you will get TypeErrors on close. See
  //  https://tobiasmarciszko.github.io/qml-binding-errors/ for discussion.
  auto data = QSharedPointer<default_data>::create();
  int flag{};
  { //  This scope forces engine to be deleted before model
    //  Instantiate QML engine before models
    QQmlApplicationEngine engine;
    // TODO: connect to PreferenceModel, read field corresponding to QPalette (Disabled, Text) field.
    engine.addImageProvider(QLatin1String("icons"), new PreferenceAwareImageProvider);
    QSharedPointer<gui_globals> globals;
    if (args.extra_init) globals = args.extra_init(engine);
    else globals = default_init(engine, data);
    (void)globals; // Unused, but keeps bound context variables from being deleted.

    QDirIterator i(":/ui", QDirIterator::Subdirectories);
    while (i.hasNext()) {
      auto f = QFileInfo(i.next());
      if (!f.isFile())
        continue;
      qDebug() << f.filePath();
    }

    static const auto default_entry = u"qrc:/qt/qml/Pepp/gui/main.qml"_qs;
    const QUrl url(args.QMLEntry.isEmpty() ? default_entry : args.QMLEntry);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
          if (!obj && url == objUrl) QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);
    flag = app.exec();

    //  Engine deleted here
  }

  //  Model deleted here
  return flag;
}

#else
int gui_main(gui_args) { return 0; }
#endif
