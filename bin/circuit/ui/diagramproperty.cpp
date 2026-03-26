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

LineProperties::LineProperties(QObject *parent) : BaseProperties(parent) {}

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

Pins::Pins(const PinType type, QObject *parent) : _type(type), QObject(parent) {
  //  Set pin locations as a percentage within space. This space may be rotated in diagram.
  // Diagram will calculate final location.
  recalcPins();
};

void Pins::recalcPins() {
  //_pins.clear();
  // for (double i = 0; i < _maxSize; ++i) _pins.append(((i + 1) / _maxSize) - 0.5);
}

bool Pins::addLine(LineProperties *line) {
  //  Limit to maximum size. Return false if too many
  if (_lines.size() <= _maxSize) {
    _lines.append(line);
    recalcPins();
    return true;
  }
  return false;
}

bool Pins::removeLine(LineProperties *line) {
  auto index = _lines.indexOf(line);
  if (index != 1) {
    //  Found
    _lines.removeAt(index);
    recalcPins();
    return true;
  }
  return false;
}

void Pins::setMaxSize(const quint16 size) {
  if (_maxSize != size) {
    _maxSize = size;
    recalcPins();
  }
}

DiagramProperties::DiagramProperties(QObject *parent) : BaseProperties(parent) {
  _outputPins.setType(Pins::PinType::Output);
}

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
  if (_properties.inputNo != v || _inputPins.pins().count() == 0) {
    _properties.inputNo = v;
    // updateInputPt();
    updateInputPinPt();

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
    updateInputPinPt();
    // updateInputPt();
    // updateOutputPt();

    emit imageChanged();
  } else if (_inputPins.pins().count() == 0) {
    //  Rotation affects line placement
    updateInputPinPt();

    emit imageChanged();
  }
}

void DiagramProperties::setKey(const PeppRect &v) {
  if (BaseProperties::setKey(v)) {
    //  Line key is based on diagram key
    //  Diagram key only changes when moving
    //  Allow lines to recalculate when diagram changes
    // updateInputKey();
    // updateOutputKey();
    emit dimensionsChanged();
  }
}

void DiagramProperties::setGridRectangle(const PeppRect &v) {
  if (BaseProperties::setGridRectangle(v)) {
    //  Movement affects line placement
    // updateInputPt();
    // updateOutputPt();

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
    y += (_baseProperties.gridRect.height() - _margin);
    break;
  // Pointing left
  case 180:
    x += _margin;
    y += _baseProperties.gridRect.height() / 2;
    break;
  //  Pointing up
  case 270:
    x += _baseProperties.gridRect.width() / 2;
    y += _margin;
    break;
    //  Pointing right
  default: x += (_baseProperties.gridRect.width() - _margin); y += _baseProperties.gridRect.height() / 2;
  }

  return {x, y};
}

void DiagramProperties::updateInputPt() {
  const auto cnt = _inputPins.size();
  if (cnt > 0) {
    //  Input is 180 degrees from output
    /*const auto orientation = (_properties.orientation + 180) % 360;
    const bool horizontal = (orientation % 180) == 0;
    const auto centerPt = input();
    qint32 pinWidth = 20;
    qint32 offset = -pinWidth / 2;
    auto maxSize = _inputPins.maxSize();
    double delta = pinWidth / maxSize;

    PeppPt pt;
    if (horizontal) {
      pt = centerPt.translated(offset, 0);
    } else {
      pt = centerPt.translated(0, offset);
    }

    //  Reset pins
    _inputPins.pins().clear();

    //  Create pin endpoints
    for (int i = 0; i < maxSize; ++i) {
      _inputPins.pins().push_back({pt, PeppSize{2, 2}});
      if (horizontal) {
        pt.setY(pt.y() + delta);
      } else {
        pt.setX(pt.x() + delta);
      }
    }*/
  }
}

void DiagramProperties::updateInputPinPt() {
  //  Input is 180 degrees from output
  auto orientation = (_properties.orientation + 180) % 360;
  bool horizontal = (orientation % 180) == 0;
  const auto centerPt = input();
  auto maxSize = _inputPins.maxSize();
  auto pinWidth = maxSize;
  auto offset = -pinWidth / 2; // + _margin;
  auto delta = pinWidth / maxSize;

  PeppPt pt;
  if (horizontal) {
    pt = centerPt.translated(0, offset);
  } else {
    pt = centerPt.translated(offset, 0);
  }

  //  Reset pins
  _inputPins.pins().clear();

  //  Create pin endpoints
  for (int i = 0; i < maxSize; ++i) {
    _inputPins.pins().emplace_back(pt, PeppSize{1, 1});
    if (horizontal) {
      pt.setY(pt.y() + delta);
    } else {
      pt.setX(pt.x() + delta);
    }
  }
}

PeppPt DiagramProperties::input() const {
  auto x = _baseProperties.gridRect.left();
  auto y = _baseProperties.gridRect.top();

  switch (_properties.orientation) {
  //  Pointing up
  case 90:
    x += _baseProperties.gridRect.width() / 2;
    y += _margin;
    break;
  // Pointing right
  case 180:
    x += (_baseProperties.gridRect.width() - _margin);
    y += _baseProperties.gridRect.height() / 2;
    break;
  //  Pointing down
  case 270:
    x += _baseProperties.gridRect.width() / 2;
    y += (_baseProperties.gridRect.height() - _margin);
    break;
  //  Pointing left
  default:
    x += _margin;
    y += _baseProperties.gridRect.height() / 2;
    break;
  }

  return {x, y};
}
