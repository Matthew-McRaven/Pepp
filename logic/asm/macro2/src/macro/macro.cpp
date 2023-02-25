#include "./macro.hpp"

macro::Parsed::Parsed(QString name, quint8 argCount, QString body,
                      QString architecture, QObject *parent)
    : QObject(parent), _name(name), _body(body), _architecture(architecture),
      _argCount(argCount) {}

QString macro::Parsed::name() const { return _name; }

QString macro::Parsed::body() const { return _body; }

quint8 macro::Parsed::argCount() const { return _argCount; }

QString macro::Parsed::architecture() const { return _architecture; }
