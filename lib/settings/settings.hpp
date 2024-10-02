#include <QObject>
#include <QtQmlIntegration>
#include "builtins/constants.hpp"

class QJSEngine;
class QQmlEngine;

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

class GeneralCategory : public Category {
  Q_OBJECT
  QML_UNCREATABLE("")
  QML_NAMED_ELEMENT(GeneralCategory)
  // "Defaults" group box
  Q_PROPERTY(builtins::Architecture defaultArch READ defaultArch WRITE setDefaultArch NOTIFY defaultArchChanged)
  Q_PROPERTY(builtins::Abstraction defaultAbstraction READ defaultAbstraction WRITE setDefaultAbstraction NOTIFY
                 defaultAbstractionChanged)
  // "Menus" group box
  Q_PROPERTY(int maxRecentFiles READ maxRecentFiles WRITE setMaxRecentFiles NOTIFY maxRecentFilesChanged)
  Q_PROPERTY(bool showMenuHotkeys READ showMenuHotkeys WRITE setShowMenuHotkeys NOTIFY showMenuHotkeysChanged)

  // "Updates" group box
  Q_PROPERTY(bool showChangeDialog READ showChangeDialog WRITE setShowChangeDialog NOTIFY showChangeDialogChanged)
public:
  explicit GeneralCategory(QObject *parent = nullptr);
  QString name() const override { return "General"; };
  QString source() const override { return "GeneralCategoryDelegate.qml"; };
  void sync() override;

  builtins::Architecture defaultArch() const;
  void setDefaultArch(builtins::Architecture arch);
  builtins::Abstraction defaultAbstraction() const;
  void setDefaultAbstraction(builtins::Abstraction abstraction);
  int maxRecentFiles() const;
  void setMaxRecentFiles(int max);
  bool showMenuHotkeys() const;
  void setShowMenuHotkeys(bool show);
  bool showChangeDialog() const;
  void setShowChangeDialog(bool show);

signals:
  void defaultArchChanged();
  void defaultAbstractionChanged();
  void maxRecentFilesChanged();
  void showMenuHotkeysChanged();
  void showChangeDialogChanged();

private:
  mutable QSettings _settings;
  const builtins::Architecture defaultDefaultArch = builtins::Architecture::PEP10;
  const builtins::Abstraction defaultDefaultAbstraction = builtins::Abstraction::ASMB5;
  bool validateMaxRecentFiles(int max) const;
  const int defaultMaxRecentFiles = 5;
  const bool defaultShowMenuHotkeys = true;
  const bool defaultShowChangeDialog = true;
};

class ThemeCategory : public Category {
  Q_OBJECT
  QML_UNCREATABLE("")
  QML_NAMED_ELEMENT(ThemeCategory)

public:
  explicit ThemeCategory(QObject *parent = nullptr);
  QString name() const override { return "Fonts & Colors"; };
};

class EditorCategory : public Category {
  Q_OBJECT
  QML_UNCREATABLE("")
  QML_NAMED_ELEMENT(ThemeCategory)

public:
  explicit EditorCategory(QObject *parent = nullptr);
  QString name() const override { return "Editor"; };
};

class KeyMapCategory : public Category {
  Q_OBJECT
  QML_UNCREATABLE("")
  QML_NAMED_ELEMENT(ThemeCategory)

public:
  explicit KeyMapCategory(QObject *parent = nullptr);
  QString name() const override { return "Key Map"; };
};

class AppSettings : public QObject {
  Q_OBJECT
  Q_PROPERTY(QList<Category *> categories READ categories CONSTANT)
  Q_PROPERTY(GeneralCategory general READ general CONSTANT)
  Q_PROPERTY(ThemeCategory theme READ theme CONSTANT)
  Q_PROPERTY(EditorCategory editor READ editor CONSTANT)
  Q_PROPERTY(KeyMapCategory keymap READ keymap CONSTANT)
  QML_SINGLETON
  QML_NAMED_ELEMENT(AppSettings)
  Q_CLASSINFO("DefaultProperty", "categories")

public:
  explicit AppSettings(QObject *parent = nullptr);
  QList<Category *> categories() const { return _categories; };
  GeneralCategory *general() const { return _general; };
  ThemeCategory *theme() const { return _theme; }
  EditorCategory *editor() const { return _editor; }
  KeyMapCategory *keymap() const { return _keymap; }
public slots:
  void sync();

private:
  GeneralCategory *_general = nullptr;
  ThemeCategory *_theme = nullptr;
  EditorCategory *_editor = nullptr;
  KeyMapCategory *_keymap = nullptr;
  QList<Category *> _categories;
};
} // namespace pepp::settings
