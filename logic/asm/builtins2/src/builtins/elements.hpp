#pragma once

#include <QSharedPointer>
#include <QString>
#include <QtCore>
#include <optional>
Q_MOC_INCLUDE("builtins/figure.hpp")
namespace builtins {
Q_NAMESPACE
enum class Architecture {
  PEP8 = 8,
  PEP9 = 90,
  PEP10 = 100,
  RISCV = 1000,
};
Q_ENUM_NS(Architecture);

class Figure;
struct Element : public QObject {
private:
  Q_OBJECT
  Q_PROPERTY(bool generated MEMBER generated);
  Q_PROPERTY(QString language MEMBER language);
  Q_PROPERTY(QString content MEMBER contents);
  Q_PROPERTY(QWeakPointer<Figure> figure MEMBER figure);

public:
  bool generated;
  QString language, contents;
  QWeakPointer<Figure> figure;
};

struct Test : public QObject {
private:
  Q_OBJECT
  Q_PROPERTY(QVariant input MEMBER input);
  Q_PROPERTY(QVariant output MEMBER output);

public:
  QVariant input, output;
};

struct Macro : public QObject {
private:
  Q_OBJECT
  Q_PROPERTY(Architecture arch MEMBER arch);
  Q_PROPERTY(QString name MEMBER name);
  Q_PROPERTY(QString text MEMBER text);

public:
  Architecture arch;
  QString name;
  QString text;
};
} // end namespace builtins
