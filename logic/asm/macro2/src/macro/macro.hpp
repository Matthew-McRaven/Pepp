#pragma once

#include <QObject>

#include "./types.hpp"
#include "macro_globals.hpp"

namespace macro {
class MACRO_EXPORT Parsed : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString name READ name CONSTANT);
  Q_PROPERTY(QString body READ body CONSTANT);
  Q_PROPERTY(quint8 argCount READ argCount CONSTANT);
  Q_PROPERTY(QString architecture READ architecture CONSTANT)
public:
  Parsed(QString name, quint8 argCount, QString body, QString architecture);
  QString name() const;
  QString body() const;
  quint8 argCount() const;
  QString architecture() const;

private:
  QString _name, _body, _architecture;
  quint8 _argCount;
};

} // namespace macro
