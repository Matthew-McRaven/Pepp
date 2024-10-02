#include <QObject>
#include <QtQmlIntegration>

class QJSEngine;
class QQmlEngine;

namespace pepp::settings {
class Category : public QObject {
  Q_OBJECT
  QML_UNCREATABLE("")
  Q_PROPERTY(QString name READ name CONSTANT)
  Q_PROPERTY(QString delegate READ delegate CONSTANT)

public:
  explicit Category(QObject *parent = nullptr);
  virtual QString name() const = 0;
  virtual QString delegate() const { return ""; };
  virtual void sync() {};
};

class GeneralCategory : public Category {
  Q_OBJECT
  QML_UNCREATABLE("")
  QML_NAMED_ELEMENT(GeneralCategory)

public:
  explicit GeneralCategory(QObject *parent = nullptr);
  QString name() const override { return "General"; };
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
