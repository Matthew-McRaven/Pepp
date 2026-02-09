#include "diagramproperty.hpp"

#include <QVariant>

quint32 DiagramProperties::_counter = 0;

DiagramProperties::DiagramProperties(QObject *parent)
    : QObject(parent)
    , _id{++_counter}
{}

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
        return rectangle();
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
    case DiagramProperty::Role::Type:
        setType(data.toInt());
        break;
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
        setRectangle(data.toRect());
        break;
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
        emit imageChanged();
    }
}

void DiagramProperties::setType(const int v)
{
    if (_type != static_cast<DiagramType::Type>(v)) {
        _type = static_cast<DiagramType::Type>(v);
        emit typeChanged();
    }
}

void DiagramProperties::setInputNo(const quint16 v)
{
    if (_inputNo != v) {
        _inputNo = v;
        emit inputChanged();
    }
}

void DiagramProperties::setOutputNo(const quint16 v)
{
    if (_outputNo != v) {
        _outputNo = v;
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
    //  Limit to 360 degrees
    const auto angle = v % 360;
    if (_orientation != angle) {
        const auto slice = static_cast<quint32>(angle / 90);

        //  Only support 90 degree changes
        _orientation = slice * 90;
        emit imageChanged();
    }
}

void DiagramProperties::setRectangle(const QRect v)
{
    if (_rect != v) {
        _rect = v;
        emit dimensionsChanged();
    }
}
