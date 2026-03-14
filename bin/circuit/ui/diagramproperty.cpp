#include "diagramproperty.hpp"

#include <QVariant>

BaseProperties::BaseProperties(QObject *parent) : QObject(parent) {}

bool BaseProperties::setId(const quint32 v) {
  if (_baseProperties.id != v) {
    _baseProperties.id = v;
    return true;
  }
  return false;
}

bool BaseProperties::setType(const DiagramType::Type v) {
  if (_baseProperties.type != v) {
    _baseProperties.type = v;
    return true;
  }
  return false;
}

bool BaseProperties::setKey(const PeppRect &v) {
  if (_baseProperties.key != v) {
    _baseProperties.key = v;
    return true;
  }
  return false;
}

bool BaseProperties::setGridRectangle(const PeppRect &v) {
  if (_baseProperties.gridRect != v) {
    _baseProperties.gridRect = v;
    return true;
  }
  return false;
}

bool BaseProperties::setSelected(const bool v) {
  if (_baseProperties.isSelected != v) {
    _baseProperties.isSelected = v;
    return true;
  }
  return false;
}

LineProperties::LineProperties(BaseProperties *parent) : BaseProperties(parent) {}

bool LineProperties::setInputPoint(const PeppPt pt) {
  if (_properties.input != pt) {
    _properties.input = pt;
    _baseProperties.gridRect = // PeppRect(_properties.output, _properties.input);
        LineProperties::recalculateGridRect(_properties.inputDiagram, _properties.outputDiagram);
    return true;
  }
  return false;
}

bool LineProperties::setOutputPoint(const PeppPt pt) {
  if (_properties.output != pt) {
    _properties.output = pt;
    _baseProperties.gridRect = // PeppRect(_properties.output, _properties.input);
        LineProperties::recalculateGridRect(_properties.inputDiagram, _properties.outputDiagram);
    return true;
  }
  return false;
}

bool LineProperties::setInputDirection(const u16 v) {
  if (_properties.inputDirection != v) {
    _properties.inputDirection = v;
    return true;
  }
  return false;
}

bool LineProperties::setOutputDirection(const u16 v) {
  if (_properties.outputDirection != v) {
    _properties.outputDirection = v;
    return true;
  }
  return false;
}

bool LineProperties::setInputDiagram(DiagramProperties *diagram) {
  if (_properties.inputDiagram != diagram) {
    _properties.inputDiagram = diagram;

    //  Diagram is linked line so that line can be
    //  informed of changes in diagram.
    diagram->setInputPoint(this);
    setKey(LineProperties::recalculateKey(_properties.inputDiagram, _properties.outputDiagram));
    return true;
  }
  return false;
}

bool LineProperties::setOutputDiagram(DiagramProperties *diagram) {
  if (_properties.outputDiagram != diagram) {
    _properties.outputDiagram = diagram;

    //  Diagram is linked line so that line can be
    //  informed of changes in diagram.
    diagram->setOutputPoint(this);
    setKey(LineProperties::recalculateKey(_properties.inputDiagram, _properties.outputDiagram));
    return true;
  }
  return false;
}

PeppRect LineProperties::recalculateKey(const DiagramProperties *inputDiagram, const DiagramProperties *outputDiagram) {
  //  This should theoretically never happen
  if (inputDiagram == nullptr && outputDiagram == nullptr) return {PeppPt{0, 0}, PeppPt{0, 0}};

  //  Address when both endpoints are present. Normal case
  if (inputDiagram != nullptr && outputDiagram != nullptr)
    return pepp::core::hull(inputDiagram->key(), outputDiagram->key());
  else if (inputDiagram != nullptr) return inputDiagram->key();
  else return outputDiagram->key();
}

PeppRect LineProperties::recalculateGridRect(const DiagramProperties *inputDiagram,
                                             const DiagramProperties *outputDiagram) {
  //  This should theoretically never happen
  if (inputDiagram == nullptr && outputDiagram == nullptr) return {PeppPt{0, 0}, PeppPt{0, 0}};

  //  Address when both endpoints are present. Normal case
  if (inputDiagram != nullptr && outputDiagram != nullptr)
    return pepp::core::hull(inputDiagram->gridRectangle(), outputDiagram->gridRectangle());
  else if (inputDiagram != nullptr) return inputDiagram->gridRectangle();
  else return outputDiagram->gridRectangle();
}

DiagramProperties::DiagramProperties(BaseProperties *parent) : BaseProperties(parent) {}

QVariant DiagramProperties::get(int role) const {
  switch (role) {
  case DiagramProperty::Role::Id: return id();
  case DiagramProperty::Role::Name: return name();
  case DiagramProperty::Role::ImageSource: return imageSource();
  case DiagramProperty::Role::Type: return type();
  case DiagramProperty::Role::InputNo: return inputNo();
  case DiagramProperty::Role::OutputNo: return outputNo();
  case DiagramProperty::Role::Selected: return selected();
  case DiagramProperty::Role::Orientation: return orientation();
  case DiagramProperty::Role::Rectangle:
    const int x = _baseProperties.key.x().lower();
    const int y = _baseProperties.key.y().lower();
    return QRect(x, y, _baseProperties.key.width(), _baseProperties.key.height());
  }

  //  Not found
  return {};
}

void DiagramProperties::set(int role, const QVariant &data) {
  switch (role) {
  case DiagramProperty::Role::Name: setName(data.toString()); break;
  case DiagramProperty::Role::ImageSource: setImageSource(data.toString()); break;
  case DiagramProperty::Role::Type: setType(static_cast<DiagramType::Type>(data.toInt())); break;
  case DiagramProperty::Role::InputNo: setInputNo(data.toInt()); break;
  case DiagramProperty::Role::OutputNo: setOutputNo(data.toInt()); break;
  case DiagramProperty::Role::Selected: setSelected(data.toBool()); break;
  case DiagramProperty::Role::Orientation: setOrientation(data.toInt()); break;
  case DiagramProperty::Role::Rectangle:
    auto oldRect = data.toRect();
    PeppPt pt{static_cast<i16>(oldRect.x()), static_cast<i16>(oldRect.y())};
    PeppSize size{static_cast<i16>(oldRect.width()), static_cast<i16>(oldRect.height())};
    PeppRect rect(pt, size);
    _baseProperties.key = rect;
    break;
  }
}

void DiagramProperties::setId(const quint32 v) {
  if (BaseProperties::setId(v)) {
    emit nameChanged();
  }
}

void DiagramProperties::setName(const QString v) {
  if (_name != v) {
    _name = v;
    emit nameChanged();
  }
}

void DiagramProperties::setImageSource(const QString v) {
  if (_imageSrc != v) {
    _imageSrc = v;

    //  Clear cached image
    _pixMap = nullptr;

    emit imageChanged();
  }
}

void DiagramProperties::setType(const DiagramType::Type v) {
  if (BaseProperties::setType(v)) {

    //  Clear cached image
    _pixMap = nullptr;

    emit typeChanged();
  }
}

void DiagramProperties::setInputNo(const quint16 v) {
  if (_properties.inputNo != v) {
    _properties.inputNo = v;
    emit inputChanged();
  }
}

void DiagramProperties::setOutputNo(const quint16 v) {
  if (_properties.outputNo != v) {
    _properties.outputNo = v;
    emit outputChanged();
  }
}

void DiagramProperties::setSelected(const bool v) {
  if (BaseProperties::setSelected(v)) {
    emit selectedChanged();
  }
}

void DiagramProperties::setOrientation(const quint32 v) {
  if (_properties.setOrientation(v)) {
    //  Clear cached image
    _pixMap = nullptr;

    //  Rotation affects line placement
    updateInputPt();
    updateOutputPt();

    emit imageChanged();
  }
}

void DiagramProperties::setKey(const PeppRect &v) {
  if (BaseProperties::setKey(v)) {
    //  Line key is based on diagram key
    //  Diagram key only changes when moving
    //  Allow lines to recalculate when diagram changes
    updateInputKey();
    updateOutputKey();
    emit dimensionsChanged();
  }
}

void DiagramProperties::setGridRectangle(const PeppRect &v) {
  if (BaseProperties::setGridRectangle(v)) {
    //  Movement affects line placement
    updateInputPt();
    updateOutputPt();

    emit dimensionsChanged();
  }
}

void DiagramProperties::setImage(QPixmap *v) {
  if (_pixMap != v) {
    _pixMap = v;
  }
}

PeppPt DiagramProperties::output() const {

  auto x = _baseProperties.gridRect.left();
  auto y = _baseProperties.gridRect.top();

  switch (_properties.orientation) {
  //  Pointing down
  case 90:
    x += _baseProperties.gridRect.width() / 2;
    y += _baseProperties.gridRect.height();
    break;
  // Pointing left
  case 180:
    //  X = 0 already
    y = _baseProperties.gridRect.height() / 2;
    break;
  //  Pointing up
  case 270:
    x = _baseProperties.gridRect.width() / 2;
    break;
    //  Pointing left
  default: x += _baseProperties.gridRect.width(); y += _baseProperties.gridRect.height() / 2;
  }

  return {x, y};
}

PeppPt DiagramProperties::input() const {
  auto x = _baseProperties.gridRect.left();
  auto y = _baseProperties.gridRect.top();

  switch (_properties.orientation) {
  //  Pointing down
  case 90: x += _baseProperties.gridRect.width() / 2; break;
  // Pointing right
  case 180:
    x += _baseProperties.gridRect.width();
    y += _baseProperties.gridRect.height() / 2;
    break;
  //  Pointing up
  case 270:
    x += _baseProperties.gridRect.width() / 2;
    y += _baseProperties.gridRect.height();
    break;
  //  Pointing left
  default:
    //  X = 0 already
    y += _baseProperties.gridRect.height() / 2;
    break;
  }

  return {x, y};
}
