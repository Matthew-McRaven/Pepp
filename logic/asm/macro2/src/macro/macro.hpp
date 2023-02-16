#pragma once

#include <QObject>

#include "./types.hpp"
namespace macro {
class Parsed : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString name READ name CONSTANT);
  Q_PROPERTY(QString body READ body CONSTANT);
  Q_PROPERTY(quint8 argCount READ argCount CONSTANT);
  Q_PROPERTY(QString architecture READ architecture CONSTANT)
public:
  Parsed(QString name, quint8 argCount, QString body, QString architecture,
         QObject *parent = nullptr);
  QString name() const;
  QString body() const;
  quint8 argCount() const;
  QString architecture() const;

private:
  QString _name, _body, _architecture;
  quint8 _argCount;
};
class Registered : public QObject {
  Q_OBJECT
  Q_PROPERTY(const Parsed *contents READ contents CONSTANT)
  Q_PROPERTY(Type type READ type CONSTANT)
public:
  // Takes ownership of contents and changes its parent to this
  Registered(Type type, Parsed *contents, QObject *parent = nullptr);
  const Parsed *contents() const;
  Type type() const;

private:
  const Parsed *_contents;
  Type _type;
};
} // namespace macro
