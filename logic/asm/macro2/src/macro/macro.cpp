#include "./macro.hpp"

macro::Parsed::Parsed(QString name, quint8 argCount, QString body,
                      QObject *parent)
    : QObject(parent), _name(name), _body(body), _argCount(argCount) {}

QString macro::Parsed::name() const { return _name; }

QString macro::Parsed::body() const { return _body; }

quint8 macro::Parsed::argCount() const { return _argCount; }

macro::Registered::Registered(Type type, Parsed *contents, QObject *parent)
    : QObject(parent), _contents(contents), _type(type) {
  contents->setParent(this);
}

const macro::Parsed *macro::Registered::contents() const { return _contents; }

macro::Type macro::Registered::type() const { return _type; }
