/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include <CLI11.hpp>
#include <QApplication>
#include <QFontDatabase>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>
#include <QtCore>
#include <iostream>
#include <kddockwidgets/Config.h>
#include <kddockwidgets/core/DockRegistry.h>
#include <kddockwidgets/core/FloatingWindow.h>
#include <kddockwidgets/core/TitleBar.h>
#include <kddockwidgets/qtquick/Platform.h>
#include <kddockwidgets/qtquick/ViewFactory.h>
#include <kddockwidgets/qtquick/views/DockWidget.h>
#include <kddockwidgets/qtquick/views/MainWindow.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include "./iconprovider.hpp"
#include "help/about/version.hpp"
//  Testing only
#include <QDirIterator>

#include "settings/settings.hpp"

#if defined(Q_OS_WASM)
const bool is_wasm = true;
#else
const bool is_wasm = false;
#endif
#ifdef PEPP_LIB_STATIC_BUILD
Q_IMPORT_PLUGIN(PeppLibPlugin)
#endif

class CustomViewFactory : public KDDockWidgets::QtQuick::ViewFactory {
public:
  ~CustomViewFactory() override;

  QUrl titleBarFilename() const override { return QUrl("qrc:/qt/qml/edu/pepp/top/DockTitleBar.qml"); }
  QUrl tabbarFilename() const override { return QUrl("qrc:/qt/qml/edu/pepp/top/DockTabBar.qml"); }
};
CustomViewFactory::~CustomViewFactory() = default;

class PeppApplication : public QApplication {
public:
  PeppApplication(int &argc, char **argv) : QApplication(argc, argv) {}
  bool event(QEvent *event) override {
    if (event->type() == QEvent::FileOpen) {
      QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
      const QUrl url = openEvent->url();
      if (url.isLocalFile()) {
        auto root = engine->rootObjects().at(0);
        auto ret = QMetaObject::invokeMethod(
            root, "onOpenFile", Q_ARG(QVariant, QVariant::fromValue(url.toLocalFile())), QVariant{}, QVariant{});
        return true;
      }
    }

    return QApplication::event(event);
  }
  QQmlApplicationEngine *engine = nullptr;
};

class QuitInterceptor : public QObject {
  Q_OBJECT
public:
  QQmlApplicationEngine *engine;

  bool eventFilter(QObject *obj, QEvent *event) override {
    if (event->type() == QEvent::Close) {
      QObject *root = engine->rootObjects().value(0, nullptr);
      if (root) QMetaObject::invokeMethod(root, "onQuit");
      event->ignore(); // block close
      return true;
    }
    return QObject::eventFilter(obj, event);
  }
};
#include "main.moc"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
PeppApplication *g_app = nullptr;
QQmlApplicationEngine *g_engine = nullptr;
#endif

int main(int argc, char **argv) {
  // Set up some useful loggers
  auto sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
  auto create = [&](const char *name) {
    auto logger = std::make_shared<spdlog::logger>(name, sink);
    spdlog::register_logger(logger);
    return logger;
  };
  auto logger_debugger = create("debugger");
  auto logger_stack_debugger = create("debugger::stack");
  logger_debugger->set_level(spdlog::level::warn);
#if defined(SPDLOG_ACTIVE_LEVEL)
  spdlog::set_level((spdlog::level::level_enum)SPDLOG_ACTIVE_LEVEL);
#endif
  // spdlog::set_level(spdlog::level::level_enum::info);

  CLI::App cli{"Pepp", "pepp"};
  cli.set_help_flag("-h,--help", "Display this help message and exit.");

  bool resetSettings = false;
  auto flagResetSettings = cli.add_flag("--reset-settings", resetSettings, "Reset settings to default");
  std::string openFile = "";
  auto flagOpenFile = cli.add_option("open-file", openFile);
  // Pass unknown flags to Qt, which is required for QML debugger.
  cli.allow_extras(true);
  try {
    cli.parse(argc, argv);
  } catch (const CLI::CallForHelp &e) {
    std::cout << cli.help() << std::endl;
    return 0;
  }

  using namespace Qt::StringLiterals;
  // Must forward args for things like QML debugger to work.
  auto remaining_argvs = cli.remaining_for_passthrough();
  std::vector<char *> new_argvs(remaining_argvs.size());
  for (int it = 0; it < remaining_argvs.size(); it++) new_argvs[it] = remaining_argvs[it].data();
  new_argvs.emplace(new_argvs.begin(), argv[0]);
  int new_argc = new_argvs.size();
#ifdef __EMSCRIPTEN__
  // clang-format off
  // Make a persistent FS for themes. `true` to load from disk 2 mem
  EM_ASM(
          // Request that IDBFS be persisted, even for localhost
      if (navigator.storage) navigator.storage.persist().then(() => {});
      if (!FS.analyzePath('/themes').exists) FS.mkdir('/themes');
      FS.mount(IDBFS, {}, '/themes');
      FS.syncfs(true, function(err) {
        if (err) console.error("Error mounting IDBFS /themes:", err);
      });
      );
  // clang-format on
  g_app = new PeppApplication(new_argc, new_argvs.data());
  PeppApplication *app_ptr = g_app;
  g_engine = new QQmlApplicationEngine;
  QQmlApplicationEngine &engine = *g_engine;
#else
  PeppApplication app(new_argc, new_argvs.data());
  PeppApplication *app_ptr = &app;
  QQmlApplicationEngine engine;
#endif

  for (QDirIterator i(":/fonts/", QDirIterator::Subdirectories); i.hasNext();)
    if (auto f = QFileInfo(i.next()); f.isFile()) QFontDatabase::addApplicationFont(f.absoluteFilePath());

  QApplication::setOrganizationName("Pepperdine University");
  QApplication::setApplicationName("Pepp");
  QApplication::setOrganizationDomain("pepp.pepperdine.edu");
  QApplication::setApplicationDisplayName("Pepp IDE");
  static auto version =
      u"%1.%2.%3"_s.arg(about::g_MAJOR_VERSION()).arg(about::g_MINOR_VERSION()).arg(about::g_PATCH_VERSION());
  QApplication::setApplicationVersion(version);
  QQuickStyle::setStyle("Fusion");
  if (resetSettings) pepp::settings::AppSettings().resetToDefault();

  // TODO: connect to PreferenceModel, read field corresponding to QPalette (Disabled, Text) field.
  engine.addImageProvider(QLatin1String("icons"), new PreferenceAwareImageProvider);

  // Configure dock widgets to use QML
  KDDockWidgets::initFrontend(KDDockWidgets::FrontendType::QtQuick);
  auto &config = KDDockWidgets::Config::self();

  // I dislike floating windows. This prevents windows from staying in a floating state after being dragged.
  KDDockWidgets::Config::self().setDragEndedFunc([] {
    const auto floatingWindows = KDDockWidgets::DockRegistry::self()->floatingWindows();
    for (auto fw : floatingWindows) {
      if (!fw->beingDeleted()) fw->titleBar()->onFloatClicked();
    }
  });
  // Make it impossible for a docking widget to be closed / maximized / floated.
  auto flags = config.flags() | KDDockWidgets::Config::Flag_TitleBarIsFocusable;
  flags.setFlag(KDDockWidgets::Config::Flag_TabsHaveCloseButton, false);
  flags.setFlag(KDDockWidgets::Config::Flag_TitleBarHasMaximizeButton, false);
  flags.setFlag(KDDockWidgets::Config::Flag_DoubleClickMaximizes, false);
  flags.setFlag(KDDockWidgets::Config::Flag_DisableDoubleClick, true);
  // flags.setFlag(KDDockWidgets::Config::Flag_DontUseUtilityFloatingWindows, true);
  flags |= KDDockWidgets::Config::Flag_HideTitleBarWhenTabsVisible;
  config.setFlags(flags);
  config.setViewFactory(new CustomViewFactory());
  KDDockWidgets::QtQuick::Platform::instance()->setQmlEngine(&engine);

  // Load main window's QML document
  const QUrl url(u"qrc:/qt/qml/Pepp/main.qml"_s);
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, app_ptr,
      [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);
  // Don't block the event loop in WASM, especially important if wasm-exceptions are enabled.
  // See: https://doc.qt.io/qt-6/wasm.html#wasm-exceptions
  // See: https://doc.qt.io/qt-6/wasm.html#application-startup-and-the-event-loop
  engine.load(url);
  app_ptr->engine = &engine;

  // Intercept close events so that we can prompt to save changes.
  auto window = qobject_cast<QWindow *>(engine.rootObjects().value(0));
  auto filter = new QuitInterceptor;
  filter->engine = &engine;
  window->installEventFilter(filter);
  // Windows signals for file open events by passing file as arg to application.
  // I already have a system to deal with file open events, so let's post an event instead.
  if (flagOpenFile) {
    auto asURL = QUrl(openFile.c_str());
    auto file_string = asURL.toLocalFile();
    // Event must be heap allocated; ownership is assumed by event queue.
    // https://doc.qt.io/qt-6/qcoreapplication.html#postEvent
    QFileOpenEvent *ev = new QFileOpenEvent(file_string);
    QCoreApplication::postEvent(QCoreApplication::instance(), ev);
  }

#ifdef __EMSCRIPTEN__
  return 0;
#else
  return app.exec();
#endif
}
