#pragma once

#include <QObject>
struct MacroParseResult : public QObject {
  Q_OBJECT;
  Q_PROPERTY(bool valid READ valid CONSTANT)
  Q_PROPERTY(QString name READ name CONSTANT)
  Q_PROPERTY(quint8 argc READ argc CONSTANT)

public:
  MacroParseResult(QObject *parent);
  MacroParseResult(QObject *parent, bool valid, QString name, quint8 argc);
  bool valid() const;
  QString name() const;
  quint8 argc() const;
  bool _valid = false;
  QString _name = "";
  quint8 _argc = 0;
};

class MacroParser : public QObject {
  Q_OBJECT
public:
  Q_INVOKABLE MacroParseResult *parse(QString arg);
};
