#pragma once
#include "elements.hpp"
#include <QtCore>
namespace builtins {
class Figure : public QObject {
  Q_OBJECT
  Q_PROPERTY(builtins::Architecture arch READ arch CONSTANT);
  Q_PROPERTY(QString chapterName READ chapterName CONSTANT);
  Q_PROPERTY(QString figureName READ figureName CONSTANT);
  Q_PROPERTY(bool isOS READ isOS WRITE setIsOS NOTIFY isOSChanged);
  Q_PROPERTY(QSharedPointer<const Figure> defaultOS READ defaultOS WRITE
                 setDefaultOS NOTIFY defaultOSChanged);
  Q_PROPERTY(const QList<QSharedPointer<const builtins::Test>> tests READ tests
                 NOTIFY testsChanged);
  Q_PROPERTY(const QMap<QString, QSharedPointer<const builtins::Element>>
                 elements READ elements NOTIFY elementsChanged);

public:
  Figure(Architecture arch, QString chapter, QString figure);

  builtins::Architecture arch() const;

  QString chapterName() const;

  QString figureName() const;

  bool isOS() const;
  bool setIsOS(bool value);

  QSharedPointer<const Figure> defaultOS() const;
  bool setDefaultOS(QSharedPointer<const Figure>);

  const QList<QSharedPointer<const builtins::Test>> tests() const;
  void addTest(QSharedPointer<builtins::Test> test);

  const QMap<QString, QSharedPointer<const builtins::Element>> elements() const;
  bool addElement(QString name, QSharedPointer<builtins::Element> element);

signals:
  void isOSChanged();
  void defaultOSChanged();
  void testsChanged();
  void elementsChanged();

private:
  const Architecture _arch;
  const QString _chapterName, _figureName;
  bool _isOS = false;
  QSharedPointer<const Figure> _defaultOS = nullptr;
  QList<QSharedPointer<const builtins::Test>> _tests = {};
  QMap<QString, QSharedPointer<const builtins::Element>> _elements = {};
};
} // end namespace builtins
