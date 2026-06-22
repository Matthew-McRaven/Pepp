/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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
#pragma once
#include <QObject>
#include <QtQmlIntegration>
#include "palette.hpp"
#include "project/architectures.hpp"
#include "project/features.hpp"
#include "project/levels.hpp"

class QJSEngine;
class QQmlEngine;

namespace builtins {
class FigureWrapper;
}

namespace pepp::settings {
class Category : public QObject {
  Q_OBJECT
  QML_UNCREATABLE("")
  Q_PROPERTY(QString name READ name CONSTANT)
  Q_PROPERTY(QString source READ source CONSTANT)

public:
  explicit Category(QObject *parent = nullptr);
  virtual QString name() const = 0;
  virtual QString source() const { return "UnimplementedCategoryDelegate.qml"; };
  // Flush all QSettings to a file
  virtual void sync() {};
  // Reset all settings to their default values
  virtual void resetToDefault() {};
  // Pull all settings from QSettings
  virtual void reload() {};
};

class RecentFile {
  Q_GADGET
  Q_PROPERTY(QString path MEMBER _path CONSTANT)
  Q_PROPERTY(int arch READ qml_arch CONSTANT)
  Q_PROPERTY(int abstraction READ qml_level CONSTANT)
  Q_PROPERTY(int features READ qml_features CONSTANT)
  QML_UNCREATABLE("")
  QML_VALUE_TYPE(recent_file)
public:
  RecentFile() = default;
  RecentFile(const QString &filePath, pepp::Architecture arch, pepp::Abstraction level, pepp::Features features)
      : _path(filePath), _arch(arch), _level(level), _features(features) {};
  RecentFile(const RecentFile &other) noexcept = default;
  RecentFile &operator=(const RecentFile &other) noexcept = default;
  QString path() const { return _path; }
  int qml_arch() const { return (int)_arch; }
  int qml_level() const { return (int)_level; }
  int qml_features() const { return (int)_features; }
  pepp::Architecture arch() const { return _arch; }
  pepp::Abstraction abstraction() const { return _level; }
  pepp::Features features() const { return _features; }

  Qt::strong_ordering operator<=>(const RecentFile &other) const;

private:
  QString _path = "";
  pepp::Architecture _arch = pepp::Architecture::NO_ARCH;
  pepp::Abstraction _level = pepp::Abstraction::NO_ABS;
  pepp::Features _features = pepp::Features::None;
};
QDataStream &operator<<(QDataStream &out, const pepp::settings::RecentFile &rf);

QDataStream &operator>>(QDataStream &in, pepp::settings::RecentFile &rf);

class GeneralCategory : public Category {
  Q_OBJECT
  QML_UNCREATABLE("")
  QML_NAMED_ELEMENT(GeneralCategory)
  // "Defaults" group box
  // When given a file with an ambiguous extension, interpret it using this architecture.
  Q_PROPERTY(int defaultEdition READ defaultEdition WRITE setDefaultEdition NOTIFY defaultEditionChanged)
  Q_PROPERTY(int defaultArch READ qml_defaultArch WRITE setDefaultArch NOTIFY defaultArchChanged)
  Q_PROPERTY(
      int defaultAbstraction READ qml_defaultAbstraction WRITE setDefaultAbstraction NOTIFY defaultAbstractionChanged)
  Q_PROPERTY(
      bool showDebugComponents READ showDebugComponents WRITE setShowDebugComponents NOTIFY showDebugComponentsChanged)
  // "Menus" group box
  Q_PROPERTY(int maxRecentFiles READ maxRecentFiles WRITE setMaxRecentFiles NOTIFY maxRecentFilesChanged)
  Q_PROPERTY(bool showMenuHotkeys READ showMenuHotkeys WRITE setShowMenuHotkeys NOTIFY showMenuHotkeysChanged)

  // "Updates" group box
  Q_PROPERTY(bool showChangeDialog READ showChangeDialog WRITE setShowChangeDialog NOTIFY showChangeDialogChanged)

  // "App Developer" group box
  Q_PROPERTY(bool allowExternalFigures READ allowExternalFigures WRITE setAllowExternalFigures NOTIFY
                 allowExternalFiguresChanged)
  Q_PROPERTY(QString externalFigureDirectory READ externalFigureDirectory WRITE setExternalFigureDirectory NOTIFY
                 externalFigureDirectoryChanged)
  // Non-GUI properties
  Q_PROPERTY(QList<RecentFile> recentFiles READ recentFiles NOTIFY recentFilesChanges)

public:
  explicit GeneralCategory(QObject *parent = nullptr);
  QString name() const override { return "General"; };
  QString source() const override { return "GeneralCategoryDelegate.qml"; };
  void sync() override;
  void resetToDefault() override;

  int defaultEdition() const;
  void setDefaultEdition(int edition);
  int qml_defaultArch() const;
  pepp::Architecture defaultArch() const;
  void setDefaultArch(int arch);
  int qml_defaultAbstraction() const;
  pepp::Abstraction defaultAbstraction() const;
  bool showDebugComponents() const;
  void setShowDebugComponents(bool show);
  void setDefaultAbstraction(int abstraction);
  int maxRecentFiles() const;
  void setMaxRecentFiles(int max);
  bool showMenuHotkeys() const;
  void setShowMenuHotkeys(bool show);
  bool showChangeDialog() const;
  void setShowChangeDialog(bool show);
  bool allowExternalFigures() const;
  void setAllowExternalFigures(bool allow);
  QString externalFigureDirectory() const;
  void setExternalFigureDirectory(const QString &path);
  QString figureDirectory() const;
  Q_INVOKABLE void pushRecentFile(const QString &fileName, pepp::Architecture arch, pepp::Abstraction level,
                                  pepp::Features features);
  Q_INVOKABLE void clearRecentFiles();
  // Really should be in a seperate class, but I only use it when touching recent files.
  Q_INVOKABLE QString fileNameFor(const QString &fullPath);
  QList<RecentFile> recentFiles() const;
signals:
  void defaultEditionChanged();
  void defaultArchChanged();
  void defaultAbstractionChanged();
  void showDebugComponentsChanged();
  void maxRecentFilesChanged();
  void showMenuHotkeysChanged();
  void showChangeDialogChanged();
  void allowExternalFiguresChanged();
  void externalFigureDirectoryChanged();
  void recentFilesChanges();

private:
  mutable QSettings _settings;
  void refreshRecentFileCache() const;
  mutable QList<RecentFile> _recentFileCache;
  const int defaultDefaultEdition = 6;
  const pepp::Architecture defaultDefaultArch = pepp::Architecture::PEP10;
  const pepp::Abstraction defaultDefaultAbstraction = pepp::Abstraction::ASMB5;
  const bool defaultShowDebugComponents = false;
  bool validateMaxRecentFiles(int max) const;
  const int defaultMaxRecentFiles = 5;
  const bool defaultShowMenuHotkeys = true;
  const bool defaultShowChangeDialog = true;
  const bool defaultAllowExternalFigures = false;
};

class ThemeCategory : public Category {
  Q_OBJECT
  QML_UNCREATABLE("")
  QML_NAMED_ELEMENT(ThemeCategory)
  Q_PROPERTY(pepp::settings::Palette *palette READ palette CONSTANT)
  Q_PROPERTY(QString themePath READ themePath WRITE setThemePath NOTIFY themePathChanged)

public:
  explicit ThemeCategory(QObject *parent = nullptr);
  QString name() const override { return "Fonts & Colors"; };
  QString source() const override { return "ThemeCategoryDelegate.qml"; };
  pepp::settings::Palette *palette() const { return _palette; };
  QString themePath() const;
  void setThemePath(const QString &path);
  // Flush all QSettings to a file
  void sync() override;
  void resetToDefault() override;
  bool loadFromPath(pepp::settings::Palette *pal, const QString &path);
signals:
  void themePathChanged();
private slots:
  void onPaletteItemChanged();

private:
  mutable QSettings _settings;
  QString _themePath;
  pepp::settings::Palette *_palette = nullptr;
};

class EditorCategory : public Category {
  Q_OBJECT
  QML_UNCREATABLE("")
  QML_NAMED_ELEMENT(EditorCategory)
  Q_PROPERTY(
      bool visualizeWhitespace READ visualizeWhitespace WRITE setVisualizeWhitespace NOTIFY visualizeWhitespaceChanged)

public:
  explicit EditorCategory(QObject *parent = nullptr);
  QString name() const override { return "Editor"; };
  QString source() const override { return "EditorCategoryDelegate.qml"; };
  void sync() override;
  void resetToDefault() override;

  bool visualizeWhitespace() const;
  void setVisualizeWhitespace(bool visualize);

signals:
  void visualizeWhitespaceChanged();

private:
  const bool _defaultVisualizeWhitespace = false;
  mutable QSettings _settings;
};

class SimulatorCategory : public Category {
  Q_OBJECT
  QML_UNCREATABLE("")
  QML_NAMED_ELEMENT(SimulatorCategory)
  Q_PROPERTY(
      int maxStepbackBufferKB READ maxStepbackBufferKB WRITE setMaxStepbackBufferKB NOTIFY maxStepbackBufferKBChanged)

public:
  explicit SimulatorCategory(QObject *parent = nullptr);
  QString name() const override { return "Simulator"; };
  QString source() const override { return "SimulatorCategoryDelegate.qml"; };
  void sync() override;
  void resetToDefault() override;

  Q_INVOKABLE int minMaxStepbackBufferKB() const;
  Q_INVOKABLE int maxMaxStepbackBufferKB() const;
  int maxStepbackBufferKB() const;
  void setMaxStepbackBufferKB(int max);

signals:
  void maxStepbackBufferKBChanged();

private:
  bool validateMaxStepbackBufferKB(int max) const;
  const int _defaultMaxStepbackBufferKB = 50;
  mutable QSettings _settings;
};

class KeyMapCategory : public Category {
  Q_OBJECT
  QML_UNCREATABLE("")
  QML_NAMED_ELEMENT(KeyMapCategory)

public:
  explicit KeyMapCategory(QObject *parent = nullptr);
  QString name() const override { return "Key Map"; };
  QString source() const override { return "KeymapCategoryDelegate.qml"; };
};

class FavoriteFigure {
  Q_GADGET
  Q_PROPERTY(int edition READ edition CONSTANT)
  Q_PROPERTY(QString chapter READ chapter CONSTANT)
  Q_PROPERTY(QString figure READ figure CONSTANT)
  QML_UNCREATABLE("")
  QML_VALUE_TYPE(favorite_figure)
public:
  FavoriteFigure() = default;
  FavoriteFigure(int edition, const QString &chapter, const QString &figure)
      : _edition(edition), _chapter(chapter), _figure(figure) {};
  FavoriteFigure(const FavoriteFigure &other) noexcept = default;
  FavoriteFigure &operator=(const FavoriteFigure &other) noexcept = default;

  int edition() const { return _edition; }
  QString chapter() const { return _chapter; }
  QString figure() const { return _figure; }

  Qt::strong_ordering operator<=>(const FavoriteFigure &other) const;
  bool operator==(const FavoriteFigure &other) const = default;

private:
  int _edition = 0;
  QString _chapter = "", _figure = "";
};

QDataStream &operator<<(QDataStream &out, const pepp::settings::FavoriteFigure &rf);
QDataStream &operator>>(QDataStream &in, pepp::settings::FavoriteFigure &rf);

class FavoriteFigureCategory : public Category {
  Q_OBJECT
  QML_NAMED_ELEMENT(FavoriteFigures)
  Q_PROPERTY(QList<FavoriteFigure> favorites READ favorites NOTIFY favoritesChanged)
public:
  explicit FavoriteFigureCategory(QObject *parent = nullptr);
  QString name() const override;
  QString source() const override;
  void sync() override;
  // Reset internal state to default without emitting a changed event.
  void resetToDefault() override;
  // Reset to default & emit a changed event.
  Q_INVOKABLE void reset();

  QList<FavoriteFigure> favorites() const;
  Q_INVOKABLE void clear();
  void addFavorite(const builtins::FigureWrapper *figure);
  void removeFavorite(const builtins::FigureWrapper *figure);
  bool contains(FavoriteFigure figure) const;
signals:
  void favoritesChanged();

private:
  void refreshFavoritesCache() const;
  mutable QList<FavoriteFigure> _favoritesCache;
  mutable QSettings _settings;
};

namespace detail {
class AppSettingsData {
public:
  static AppSettingsData *getInstance();
  QList<Category *> categories() const { return _categories; };
  GeneralCategory *general() const { return _general; };
  ThemeCategory *theme() const { return _theme; };
  pepp::settings::Palette *themePalette() const { return _theme->palette(); };
  EditorCategory *editor() const { return _editor; };
  SimulatorCategory *simulator() const { return _simulator; };
  KeyMapCategory *keymap() const { return _keymap; };
  FavoriteFigureCategory *favorites() const { return _favorites; };

private:
  AppSettingsData();
  GeneralCategory *_general = nullptr;
  ThemeCategory *_theme = nullptr;
  EditorCategory *_editor = nullptr;
  SimulatorCategory *_simulator = nullptr;
  KeyMapCategory *_keymap = nullptr;
  QList<Category *> _categories;
  FavoriteFigureCategory *_favorites = nullptr;
};

} // namespace detail
class AppSettings : public QObject {
  Q_OBJECT
  Q_PROPERTY(QList<Category *> categories READ categories NOTIFY categoriesChanged)
  Q_PROPERTY(GeneralCategory *general READ general NOTIFY generalChanged)
  Q_PROPERTY(ThemeCategory *theme READ theme NOTIFY themeChanged)
  // alias to make access themeing take fewer keystrokes
  Q_PROPERTY(pepp::settings::Palette *extPalette READ themePalette NOTIFY themePaletteChanged)
  Q_PROPERTY(EditorCategory *editor READ editor NOTIFY editorChanged)
  Q_PROPERTY(SimulatorCategory *simulator READ simulator NOTIFY simulatorChanged)
  Q_PROPERTY(KeyMapCategory *keymap READ keymap NOTIFY keymapChanged)
  Q_PROPERTY(FavoriteFigureCategory *favorites READ favorites NOTIFY favoritesChanged)
  QML_NAMED_ELEMENT(NuAppSettings)
  Q_CLASSINFO("DefaultProperty", "categories")

public:
  explicit AppSettings(QObject *parent = nullptr);
  QList<Category *> categories() const;
  GeneralCategory *general() const;
  ThemeCategory *theme() const;
  pepp::settings::Palette *themePalette() const;
  EditorCategory *editor() const;
  SimulatorCategory *simulator() const;
  KeyMapCategory *keymap() const;
  FavoriteFigureCategory *favorites() const;
  Q_INVOKABLE void loadPalette(const QString &path);
  Q_INVOKABLE void resetToDefault();
public slots:
  void sync();
signals:
  void categoriesChanged();
  void generalChanged();
  void themeChanged();
  void themePaletteChanged();
  void editorChanged();
  void simulatorChanged();
  void keymapChanged();
  void favoritesChanged();

private:
  detail::AppSettingsData *_data = nullptr;
};
} // namespace pepp::settings
Q_DECLARE_METATYPE(pepp::settings::RecentFile)
