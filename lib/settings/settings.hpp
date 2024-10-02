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

class AppSettings : public QObject {
  Q_OBJECT
  Q_PROPERTY(QList<Category *> categories READ categories CONSTANT)
  Q_PROPERTY(GeneralCategory general READ general CONSTANT)
  Q_PROPERTY(ThemeCategory theme READ theme CONSTANT)
  QML_SINGLETON
  QML_NAMED_ELEMENT(AppSettings)
  Q_CLASSINFO("DefaultProperty", "categories")

public:
  explicit AppSettings(QObject *parent = nullptr);
  QList<Category *> categories() const { return _categories; };
  GeneralCategory *general() const { return _general; };
  ThemeCategory *theme() const { return _theme; }

private:
  GeneralCategory *_general = nullptr;
  ThemeCategory *_theme = nullptr;
  QList<Category *> _categories;
};
} // namespace pepp::settings
