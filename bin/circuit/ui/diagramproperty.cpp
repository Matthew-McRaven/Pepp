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
    emit selectedChanged();
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
};


bool Pins::addLine(LineProperties *line) {
  //  Limit to maximum size. Return false if too many
  if (_lines.size() <= _maxSize) {
    _lines.append(line);

    //  Use pin aligned with current line position
    const auto pt = _pins.at(_lines.count() - 1);
    if (_type == PinType::Input) line->setInputPoint(pt.top_left());
    else line->setOutputPoint(pt.top_left());

    return true;
  }
  return false;
}

bool Pins::removeLine(LineProperties *line) {
  auto index = _lines.indexOf(line);
  if (index != 1) {
    //  Found
    _lines.removeAt(index);
    return true;
  }
  return false;
}

void Pins::setMaxSize(const quint16 size) {
  if (_maxSize != size) {
    _maxSize = size;
  }
}

DiagramProperties::DiagramProperties(QObject *parent) : BaseProperties(parent) {
  _outputPins.setType(Pins::PinType::Output);
  _outputPins.setMaxSize(1);
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

void DiagramProperties::setType(const DiagramType::Type v) {
  if (BaseProperties::setType(v)) {
    //  Clear cached image
    _mipmapKey.value = 0;
    emit typeChanged();
  }
}

void DiagramProperties::setInputNo(const quint16 v) {
  if (inputNo() != v) {
    _inputPins.setMaxSize(v);
    updateInputPinPt();

    emit inputChanged();
  }
}

void DiagramProperties::setOutputNo(const quint16 v) {
  if (outputNo() != v) {
    _outputPins.setMaxSize(v);
    updateOutputPinPt();
    emit outputChanged();
  }
}

void DiagramProperties::setSelected(const bool v) {
  if (BaseProperties::setSelected(v)) {
    emit selectedChanged();
  }
}

void DiagramProperties::setOrientation(const quint32 v) {
  //  Limit to 360 degrees
  const auto angle = v % 360;
  if (_properties.orientation == angle) {
    //  Nothing changed
    return;
  }

  //  Diagram changed
  const auto slice = static_cast<u32>(angle / 90);

  //  Only support 90 degree changes
  _properties.orientation = slice * 90;

  //  Clear cached image
  _mipmapKey.value = 0;

  //  Rotation affects line placement
  updateInputPinPt();
  updateOutputPinPt();

  emit imageChanged();
}

void DiagramProperties::setKey(const PeppRect &v) {
  if (BaseProperties::setKey(v)) {
    emit dimensionsChanged();
  }
}

void DiagramProperties::setGridRectangle(const PeppRect &v) {
  if (BaseProperties::setGridRectangle(v)) {
    //  Line key is based on diagram key
    //  Diagram key only changes when moving
    //  Allow lines to recalculate when diagram changes
    updateInputPinPt();
    updateOutputPinPt();

    emit dimensionsChanged();
  }
}

void DiagramProperties::setImageKey(u32 key) { _mipmapKey.value = key; }

void DiagramProperties::setTypesafeImageKey(const schematic::MipmapStoreKey &key) { _mipmapKey = key; }

void DiagramProperties::updateOutputPinPt() {
  //  Output at 0 degrees exits to right
  auto orientation = _properties.orientation % 360;
  bool horizontal = (orientation % 180) == 0;
  const auto centerPt = output();
  auto maxSize = _outputPins.maxSize();
  auto pinWidth = maxSize;
  auto offset = -pinWidth / 2;
  auto delta = pinWidth / maxSize;

  PeppPt pt;
  if (horizontal) {
    pt = centerPt.translated(0, offset);
  } else {
    pt = centerPt.translated(offset, 0);
  }

  //  Reset pins
  _outputPins.pins().clear();

  const auto lines = _outputPins.lines().size();

  //  Create pin endpoints
  for (int i = 0; i < maxSize; ++i) {
    _outputPins.pins().emplace_back(pt, PeppSize{1, 1});
    //  Update lines attached to pins
    if (i < lines) {
      auto line = _outputPins.lines().at(i);
      line->setOutputPoint(pt);
    }
    if (horizontal) {
      pt.setY(pt.y() + delta);
    } else {
      pt.setX(pt.x() + delta);
    }
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

void DiagramProperties::updateInputPinPt() {
  //  Input is 180 degrees from output
  auto orientation = (_properties.orientation + 180) % 360;
  bool horizontal = (orientation % 180) == 0;
  const auto centerPt = input();
  auto maxSize = _inputPins.maxSize();
  auto pinWidth = maxSize;
  auto offset = -pinWidth / 2;
  auto delta = pinWidth / maxSize;

  PeppPt pt;
  if (horizontal) {
    pt = centerPt.translated(0, offset);
  } else {
    pt = centerPt.translated(offset, 0);
  }

  //  Reset pins
  _inputPins.pins().clear();

  const auto lines = _inputPins.lines().size();

  //  Create pin endpoints
  for (int i = 0; i < maxSize; ++i) {
    //  Update pin location
    _inputPins.pins().emplace_back(pt, PeppSize{1, 1});
    //  Update lines attached to pins
    if (i < lines) {
      auto line = _inputPins.lines().at(i);
      line->setInputPoint(pt);
    }

    //  Update variables for next loop
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
