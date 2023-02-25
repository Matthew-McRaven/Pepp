#include "./macro.hpp"

macro::Parsed::Parsed(QString name, quint8 argCount, QString body,
                      QString architecture, QObject *parent)
    : QObject(parent), _name(name), _body(body), _architecture(architecture),
      _argCount(argCount) {}

QString macro::Parsed::name() const { return _name; }

QString macro::Parsed::body() const { return _body; }

quint8 macro::Parsed::argCount() const { return _argCount; }

QString macro::Parsed::architecture() const { return _architecture; }

macro::Registered::Registered(types::Type type, Parsed *contents,
                              QObject *parent)
    : QObject(parent), _contents(contents), _type(type) {
  contents->setParent(this);
}

const macro::Parsed *macro::Registered::contents() const { return _contents; }

macro::types::Type macro::Registered::type() const { return _type; }
