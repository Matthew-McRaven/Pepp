#include "qml_overlays.hpp"

pepp::QMLOverlay::QMLOverlay(QRect location, QObject *parent) : QObject(parent), _location(location) {}

int pepp::QMLOverlay::type() { return OVERLAY_NONE; }

QString pepp::ClockOverlay::label() const { return _label; }

bool pepp::ClockOverlay::value() const { return _value; }

void pepp::ClockOverlay::setValue(bool value) {
  if (_value != value) {
    _value = value;
    emit valueChanged();
  }
}

pepp::ClockOverlay::ClockOverlay(QRect location, QString label, QObject *parent)
    : QMLOverlay(location, parent), _label(label), _value(false) {}

int pepp::ClockOverlay::type() { return OVERLAY_CLOCK; }

pepp::TristateOverlay::TristateOverlay(QRect location, QString label, int max_value, QObject *parent)
    : QMLOverlay(location, parent), _max_value(max_value), _value(-1), _label(label) {}

int pepp::TristateOverlay::type() { return OVERLAY_TRISTATE; }

QString pepp::TristateOverlay::label() const { return _label; }

int pepp::TristateOverlay::value() const { return _value; }

void pepp::TristateOverlay::setValue(int value) {
  if (_value != value) {
    _value = value;
    emit valueChanged();
  }
}

int pepp::TristateOverlay::max_value() const { return _max_value; }

pepp::MonoTextOverlay::MonoTextOverlay(QRect location, QString label, Qt::Alignment halign, QObject *parent)
    : QMLOverlay(location, parent), _halign(halign), _label(label) {}

int pepp::MonoTextOverlay::type() { return OVERLAY_MONO_TEXT; }

QString pepp::MonoTextOverlay::label() const { return _label; }

int pepp::MonoTextOverlay::requestedHAlign() const { return _halign; }
