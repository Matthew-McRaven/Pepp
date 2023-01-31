#pragma once

#include <QObject>
#include <QtCore>
Q_MOC_INCLUDE("builtins/figure.hpp")

namespace builtins {
class Figure;
class Macro;

class Book : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString name READ name CONSTANT);
  Q_PROPERTY(const QList<QSharedPointer<builtins::Figure>> figures READ figures
                 NOTIFY figuresChanged);
  Q_PROPERTY(const QList<QSharedPointer<builtins::Macro>> macros READ macros
                 NOTIFY macrosChanged);

public:
  explicit Book(QString name);
  QString name() const;
  const QList<QSharedPointer<builtins::Figure>> figures() const;
  QSharedPointer<const builtins::Figure> findFigure(QString chapter,
                                                    QString figure) const;
  bool addFigure(QSharedPointer<builtins::Figure> figure);
  const QList<QSharedPointer<builtins::Macro>> macros() const;
  QSharedPointer<const builtins::Macro> findMacro(QString name) const;
  bool addMacro(QSharedPointer<builtins::Macro> macro);
signals:
  void figuresChanged();
  void macrosChanged();

private:
  QString _name;
  QList<QSharedPointer<builtins::Figure>> _figures = {};
  QList<QSharedPointer<builtins::Macro>> _macros = {};
};
} // end namespace builtins
