#include "componentwrapper.hpp"

#include "schematic/component.hpp"
#include "schematic/orient.hpp"

ComponentWrapper::ComponentWrapper(QObject *parent) : QObject(parent) {}

void ComponentWrapper::setComponent(Component *component) {
  if (_component != component) {
    _component = component;
    emit componentChanged();
  }
}

quint32 ComponentWrapper::id() const {
  if (_component != nullptr) {
    return _component->id().value;
  }
  return 0;
}
// QString name() const { return _name; }
// int type() const { return static_cast<int>(_type); }
quint16 ComponentWrapper::inputNo() const {
  if (_component != nullptr) {
    return _component->input_pin_count();
  }
  return 0;
}

quint16 ComponentWrapper::outputNo() const {
  if (_component != nullptr) {
    return _component->output_pin_count();
  }
  return 0;
}

/*void ComponentWrapper::setType(const int v)
{
  if (_type != static_cast<DiagramType::Type>(v)) {
    _type = static_cast<DiagramType::Type>(v);

           //  Clear cached image
    _pixMap = nullptr;

    emit typeChanged();
  }
}*/
