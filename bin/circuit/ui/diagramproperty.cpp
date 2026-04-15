#include "diagramproperty.hpp"

#include <QVariant>

BaseProperties::BaseProperties(QObject *parent) : QObject(parent) {}

bool BaseProperties::setSelected(const bool v) {
  if (_isSelected != v) {
    _isSelected = v;
    emit selectedChanged();
    return true;
  }
  return false;
}

LineProperties::LineProperties(QObject *parent) : BaseProperties(parent) {}

DiagramProperties::DiagramProperties(QObject *parent) : BaseProperties(parent) {}
