#include "diagramproperty.hpp"

#include <QVariant>

DiagramProperties::DiagramProperties(QObject *parent) : QObject(parent), _properties(new DiagramProperty) {}

QVariant DiagramProperties::get(int role) const
{
    switch (role) {
    case DiagramProperty::Role::Id:
        return id();
    case DiagramProperty::Role::Name:
        return name();
    case DiagramProperty::Role::ImageSource:
        return imageSource();
    case DiagramProperty::Role::Type:
        return type();
    case DiagramProperty::Role::InputNo:
        return inputNo();
    case DiagramProperty::Role::OutputNo:
        return outputNo();
    case DiagramProperty::Role::Selected:
        return selected();
    case DiagramProperty::Role::Orientation:
        return orientation();
    case DiagramProperty::Role::Rectangle:
      const int x = _properties->key.x().lower();
      const int y = _properties->key.y().lower();
      return QRect(x, y, _properties->key.width(), _properties->key.height());
    }

    //  Not found
    return {};
}

void DiagramProperties::set(int role, const QVariant &data)
{
    switch (role) {
    case DiagramProperty::Role::Name:
        setName(data.toString());
        break;
    case DiagramProperty::Role::ImageSource:
        setImageSource(data.toString());
        break;
    case DiagramProperty::Role::Type: setType(static_cast<DiagramType::Type>(data.toInt())); break;
    case DiagramProperty::Role::InputNo:
        setInputNo(data.toInt());
        break;
    case DiagramProperty::Role::OutputNo:
        setOutputNo(data.toInt());
        break;
    case DiagramProperty::Role::Selected:
        setSelected(data.toBool());
        break;
    case DiagramProperty::Role::Orientation:
        setOrientation(data.toInt());
        break;
    case DiagramProperty::Role::Rectangle:
      auto oldRect = data.toRect();
      PeppPt pt{static_cast<i16>(oldRect.x()), static_cast<i16>(oldRect.y())};
      PeppSize size{static_cast<i16>(oldRect.width()), static_cast<i16>(oldRect.height())};
      PeppRect rect(pt, size);
      _properties->key = rect;
      break;
    }
}

void DiagramProperties::setId(const quint32 v) {
  if (_properties->id != v) {
    _properties->id = v;
    emit nameChanged();
  }
}

void DiagramProperties::setName(const QString v)
{
    if (_name != v) {
        _name = v;
        emit nameChanged();
    }
}

void DiagramProperties::setImageSource(const QString v)
{
    if (_imageSrc != v) {
        _imageSrc = v;

        //  Clear cached image
        _pixMap = nullptr;

        emit imageChanged();
    }
}

void DiagramProperties::setType(const DiagramType::Type v) {
  if (_properties->type != v) {
    _properties->type = v;

    //  Clear cached image
    _pixMap = nullptr;

    emit typeChanged();
  }
}

void DiagramProperties::setInputNo(const quint16 v)
{
  if (_properties->inputNo != v) {
    _properties->inputNo = v;
    emit inputChanged();
  }
}

void DiagramProperties::setOutputNo(const quint16 v)
{
  if (_properties->outputNo != v) {
    _properties->outputNo = v;
    emit outputChanged();
  }
}

void DiagramProperties::setSelected(const bool v)
{
    if (_isSelected != v) {
        _isSelected = v;
        emit selectedChanged();
    }
}

void DiagramProperties::setOrientation(const quint32 v)
{
  if (_properties->setOrientation(v)) {
    //  Clear cached image
    _pixMap = nullptr;
    emit imageChanged();
  }
}

void DiagramProperties::setKey(const PeppRect &v) {
  if (_properties->key != v) {
    _properties->key = v;
    emit dimensionsChanged();
  }
}

void DiagramProperties::setGridRectangle(const PeppRect &v) {
  if (_gridRect != v) {
    _gridRect = v;
    emit dimensionsChanged();
  }
}

void DiagramProperties::setImage(QPixmap *v)
{
    if (_pixMap != v) {
        _pixMap = v;
    }
}
