#include <QObject>
#include <QtQmlIntegration>

class QJSEngine;
class QQmlEngine;

class GeneralCategory : public QObject {
  Q_OBJECT
  QML_UNCREATABLE("")
  QML_NAMED_ELEMENT(GeneralCategory)
  Q_PROPERTY(QString name READ name CONSTANT)
public:
  explicit GeneralCategory(QObject *parent = nullptr);
  QString name() const { return "General"; };
};

class AppSettings : public QObject {
  Q_OBJECT
  // Q_PROPERTY(GeneralPreferences *general READ general CONSTANT)
  Q_PROPERTY(QList<QObject *> categories READ categories CONSTANT)
  QML_SINGLETON
  QML_NAMED_ELEMENT(AppSettings)
  Q_CLASSINFO("DefaultProperty", "categories")

public:
  explicit AppSettings(QObject *parent = nullptr);
  QList<QObject *> categories() const { return _categories; };

private:
  GeneralCategory *_p = nullptr;
  QList<QObject *> _categories;
};
