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
#include <QFontDatabase>
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

#ifdef __EMSCRIPTEN__
QApplication *app = nullptr;
#endif
int gui_main(const gui_args &args) {
  // Must forward args for things like QML debugger to work.
  int argc = args.argvs.size();
  std::vector<char *> argvs(argc);
  // Must make copy of strings, since argvs should be editable.
  std::vector<std::string> arg_strs = args.argvs;
  for (int it = 0; it < argc; it++) argvs[it] = arg_strs[it].data();
#ifdef __EMSCRIPTEN__
  app = new QApplication(argc, argvs.data());
#else
  QApplication app(argc, argvs.data());
#endif
  QFontDatabase::addApplicationFont(":/fonts/mono/CourierPrime-Regular.ttf");
  QFontDatabase::addApplicationFont(":/fonts/mono/CourierPrime-Italic.ttf");
  QFontDatabase::addApplicationFont(":/fonts/mono/CourierPrime-Bold.ttf");
  QFontDatabase::addApplicationFont(":/fonts/mono/CourierPrime-BoldItalic.ttf");
  // Helper to enumerate all application fonts.
  /*for (const QString &family : QFontDatabase::families()) {
    qDebug() << u"Family: %1"_qs.arg(family).toStdString().c_str();
    const QStringList fontStyles = QFontDatabase::styles(family);
    for (const QString &style : fontStyles) qDebug() << u"  style: %1"_qs.arg(style).toStdString().c_str();
  }*/

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
  QQmlApplicationEngine engine;
  // TODO: connect to PreferenceModel, read field corresponding to QPalette (Disabled, Text) field.
  engine.addImageProvider(QLatin1String("icons"), new PreferenceAwareImageProvider);
  QSharedPointer<gui_globals> globals;
  if (args.extra_init) globals = args.extra_init(engine);
  else globals = default_init(engine, data);
  (void)globals; // Unused, but keeps bound context variables from being deleted.

  /*QDirIterator i(":/ui", QDirIterator::Subdirectories);
  while (i.hasNext()) {
    auto f = QFileInfo(i.next());
    if (!f.isFile())
      continue;
    qDebug() << f.filePath();
  }*/

  static const auto default_entry = u"qrc:/qt/qml/Pepp/gui/main.qml"_qs;
  const QUrl url(args.QMLEntry.isEmpty() ? default_entry : args.QMLEntry);
#ifdef __EMSCRIPTEN__
  QApplication *app_ptr = app;
#else
  QApplication *app_ptr = &app;
#endif

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, app_ptr,
      [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);
  engine.load(url);
  // Don't block the event loop in WASM, especially important if wasm-exceptions are enabled.
  // See: https://doc.qt.io/qt-6/wasm.html#wasm-exceptions
  // See: https://doc.qt.io/qt-6/wasm.html#application-startup-and-the-event-loop
#ifdef __EMSCRIPTEN__
  return 0;
#else
  return app.exec();
#endif
}

#else
int gui_main(gui_args) { return 0; }
#endif
