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
#include "../iconprovider.hpp"
#include "about/version.hpp"
#include "preferences/preferencemodel.hpp"
#include "preferences/theme.hpp"
//  Testing only
#include <QDirIterator>

Q_IMPORT_PLUGIN(PeppLibPlugin)

struct default_data : public gui_globals {
  default_data() : pm(&theme) {}
  ~default_data() override = default;
  Theme theme;
  PreferenceModel pm;
  QTimer interval;
};

void default_init(QQmlApplicationEngine &engine, default_data *data) {

  //  Connect models
  auto *ctx = engine.rootContext();
  ctx->setContextProperty("PreferenceModel", &data->pm);
  ctx->setContextProperty("Theme", &data->theme);
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
QApplication *g_app = nullptr;
gui_globals *g_globals = nullptr;
QQmlApplicationEngine *g_engine = nullptr;
#endif

int gui_main(const gui_args &args) {
  using namespace Qt::StringLiterals;
  // Must forward args for things like QML debugger to work.
  int argc = args.argvs.size();
  std::vector<char *> argvs(argc);
  // Must make copy of strings, since argvs should be editable.
  std::vector<std::string> arg_strs = args.argvs;
  for (int it = 0; it < argc; it++) argvs[it] = arg_strs[it].data();
#ifdef __EMSCRIPTEN__
  g_app = new QApplication(argc, argvs.data());
#else
  QApplication app(argc, argvs.data());
#endif
  QFontDatabase::addApplicationFont(":/fonts/mono/CourierPrime-Regular.ttf");
  QFontDatabase::addApplicationFont(":/fonts/mono/CourierPrime-Italic.ttf");
  QFontDatabase::addApplicationFont(":/fonts/mono/CourierPrime-Bold.ttf");
  QFontDatabase::addApplicationFont(":/fonts/mono/CourierPrime-BoldItalic.ttf");
  // Helper to enumerate all application fonts.
  /*for (const QString &family : QFontDatabase::families()) {
    qDebug() << u"Family: %1"_s.arg(family).toStdString().c_str();
    const QStringList fontStyles = QFontDatabase::styles(family);
    for (const QString &style : fontStyles) qDebug() << u"  style: %1"_s.arg(style).toStdString().c_str();
  }*/

  QApplication::setOrganizationName("Pepperdine University");
  QApplication::setApplicationName("Pep/10");
  QApplication::setOrganizationDomain("pep.pepperdine.edu");
  QApplication::setApplicationDisplayName("Pep/10 IDE");
  static auto version =
      u"%1.%2.%3"_s.arg(about::g_MAJOR_VERSION()).arg(about::g_MINOR_VERSION()).arg(about::g_PATCH_VERSION());
  QApplication::setApplicationVersion(version);
  QQuickStyle::setStyle("Fusion");

#ifdef __EMSCRIPTEN__
  // Need to keep pointer to non-downcast type for default_init
  auto tmp = new default_data;
  // Global data must outlive engine, or there will be errors, see comment below.
  g_globals = tmp;
  g_engine = new QQmlApplicationEngine;
  QQmlApplicationEngine &engine = *g_engine;

  default_init(engine, tmp);
#else
  // Data must outlive QML engine, or you will get TypeErrors on close.
  // These errors may also appear to be accesses to undefined properties. See
  // https://tobiasmarciszko.github.io/qml-binding-errors/ for discussion.
  auto globals = QSharedPointer<default_data>::create();
  QQmlApplicationEngine engine;

  default_init(engine, globals.get());
  (void)globals; // Unused, but keeps bound context variables from being removed via DCA.
#endif

  // TODO: connect to PreferenceModel, read field corresponding to QPalette (Disabled, Text) field.
  engine.addImageProvider(QLatin1String("icons"), new PreferenceAwareImageProvider);

  /*QDirIterator i(":/ui", QDirIterator::Subdirectories);
  while (i.hasNext()) {
    auto f = QFileInfo(i.next());
    if (!f.isFile())
      continue;
    qDebug() << f.filePath();
  }*/

  static const auto default_entry = u"qrc:/qt/qml/Pepp/gui/main.qml"_s;
  const QUrl url(args.QMLEntry.isEmpty() ? default_entry : args.QMLEntry);
#ifdef __EMSCRIPTEN__
  QApplication *app_ptr = g_app;
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
  // Request that IDB be persisted, even for localhost
  EM_ASM(if (navigator.storage) navigator.storage.persist().then(function(persistent) {
    if (persistent) {
      console.log("Storage will not be cleared except by explicit user action.");
    } else {
      console.log("Storage may be cleared by the browser under storage pressure.");
    }
  }););
  // Make a persistent FS for themes. `true` to load from disk 2 mem
  // clang-format off
  EM_ASM(
    if (!FS.analyzePath('/themes').exists) FS.mkdir('/themes');
    FS.mount(IDBFS, {}, '/themes');
    FS.syncfs(true, function(err) {
      if (err) console.error("Error mounting IDBFS /themes:", err);
      else {
        console.log("mounted IDBFS /themes");
          console.log('Files in /themes:', FS.readdir('/themes'));
      }
    });
  );
  // clang-format on
  return 0;
#else
  return app.exec();
#endif
}

#else
int gui_main(gui_args) { return 0; }
#endif
