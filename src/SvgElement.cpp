#include "SvgElement.hpp"

//	Standard library
#include <string>

/*
From w3.org: https://www.w3.org/TR/SVG2/types.html#InterfaceSVGElement
    
All of the SVG DOM interfaces that correspond directly to elements in the
SVG language (such as the SVGPathElement interface for the ‘path’ element)
derive from the SVGElement interface.
*/

//	Public interface
SvgElement::SvgElement(const std::string &xmlName, const std::string &value)
    : SvgElement()
{
    _xmlName = xmlName;
    _value = value;
}

//  Generic Dom fields
SvgElement::SvgType SvgElement::elementType() const
{
    return _elementType;
}
void SvgElement::setElementType(SvgElement::SvgType elementType)
{
    _elementType = elementType;
}

const std::string &SvgElement::xmlName() const
{
    return _xmlName;
}
void SvgElement::setXmlName(const std::string &xmlName)
{
    _xmlName = xmlName;
}
const std::string &SvgElement::value() const
{
    return _value;
}
void SvgElement::setValue(const std::string &value)
{
    _value = value;
}

//  Svg specific fields
const std::string &SvgElement::id() const
{
    return _id;
}
void SvgElement::setId(const std::string &id)
{
    _id = id;
}
const std::string &SvgElement::className() const
{
    return _className;
}
void SvgElement::setClassName(const std::string &className)
{
    _className = className;
}
const std::string &SvgElement::title() const
{
    return (_title == nullptr) ? _empty : _title->xmlName();
}
void SvgElement::setTitle(const std::string &title)
{
    //  Add logic later to create element when missing
    if (_title != nullptr) {
        _title->setValue(title);
    }
}
const std::string &SvgElement::metadata() const
{
    return (_metadata == nullptr) ? _empty : _metadata->xmlName();
}
void SvgElement::setMetadata(const std::string &metadata)
{
    //  Add logic later to create element when missing
    if (_metadata != nullptr) {
        _metadata->setValue(metadata);
    }
}
const std::string &SvgElement::desc() const
{
    return (_desc == nullptr) ? _empty : _desc->xmlName();
}
void SvgElement::setDesc(const std::string &desc)
{
    //  Add logic later to create element when missing
    if (_desc != nullptr) {
        _desc->setValue(desc);
    }
}

//  Dimension accessors
auto SvgElement::x() const
{
    return _x.value;
}
void SvgElement::setX(double x)
{
    _x.value = x;
}
void SvgElement::setX(const std::string_view sv)
{
    _x.fromString(sv);
}
auto SvgElement::y() const
{
    return _y.value;
}
void SvgElement::setY(double y)
{
    _y.value = y;
}
void SvgElement::setY(const std::string_view sv)
{
    _y.fromString(sv);
}
auto SvgElement::width() const
{
    return _width.value;
}
void SvgElement::setWidth(double width)
{
    _width.value = std::max(width, 0.0);
}
void SvgElement::setWidth(const std::string_view sv)
{
    _width.fromString(sv);
}
auto SvgElement::height() const
{
    return _height;
}
void SvgElement::setHeight(double height)
{
    _height.value = std::max(height, 0.0);
}
void SvgElement::setHeight(const std::string_view sv)
{
    _height.fromString(sv);
}

//  Generic elements that capture unprocessed svg data
const XmlAttributes &SvgElement::attributes() const
{
    return _attributes;
}
XmlAttributes &SvgElement::attributes()
{
    return _attributes;
}

const std::list<SvgElement *> &SvgElement::children() const
{
    return _elements;
}
std::list<SvgElement *> &SvgElement::children()
{
    return _elements;
}

void SvgElement::appendChild(SvgElement *child)
{
    //  Certain data is contained in elements. Save
    //  pointer to allow future programitic updates.
    if (child->xmlName() == "desc"s) {
        _desc = child;
    } else if (child->xmlName() == "metadata"s) {
        _metadata = child;
    } else if (child->xmlName() == "title"s) {
        _title = child;
    }
    //  All elements are saved, including special elements above.
    //  Used for persistence to Xml.
    _elements.push_back(child);
}

void SvgElement::toXml(SvgRope &output) const
{
    //  Id currently has element name. Change when attributes are supported
    if (_elementType == SvgElement::SvgType::DomComment) {
        std::string buffer = std::format("<!--{}-->", _value);
        output.push_back(std::move(buffer));
        //  Comments cannot have children or attributes
        return;
    }
    output.push_back("<" + _xmlName);

    attributeXml(output);

    //  No child elements and no values, add end tag
    if (_elements.empty() && _value.empty()) {
        output.push_back(" />");
        return;
    }
    output.push_back(">");

    //	Save value, if present
    if (!_value.empty())
        //  Pass copy
        output.push_back(std::string(_value));

    for (const auto *element : _elements) {
        element->toXml(output);
    }
    //  After child elements, add closing element
    output.push_back("</" + _xmlName + ">");
}

bool SvgElement::setAttribute(const std::string &key, const std::string &value)
{
    if (key == "id"s) {
        _id = value;
        return true;
    }
    if (key == "x"s) {
        return _x.fromString(value);
    }
    if (key == "y"s) {
        return _y.fromString(value);
    }
    if (key == "width"s) {
        return _width.fromString(value);
    }
    if (key == "height"s) {
        return _height.fromString(value);
    }

    //  Cache attributes that are not manipulated above.
    _attributes.add(key, value);
    return true;
}

bool SvgElement::attributeXml(SvgRope &output) const
{
    std::string buffer;
    bool hasAttributes = false;
    if (!_id.empty()) {
        hasAttributes = true;
        buffer = std::format(" id=\"{}\"", _id);
        output.push_back(std::move(buffer));
    }
    if (!_x.empty()) {
        hasAttributes = true;
        buffer = std::format(" x=\"{}\"", _x.toString());
        output.push_back(std::move(buffer));
    }
    if (!_y.empty()) {
        hasAttributes = true;
        buffer = std::format(" y=\"{}\"", _y.toString());
        output.push_back(std::move(buffer));
    }
    if (!_width.empty()) {
        hasAttributes = true;
        buffer = std::format(" width=\"{}\"", _width.toString());
        output.push_back(std::move(buffer));
    }
    if (!_height.empty()) {
        hasAttributes = true;
        buffer = std::format(" height=\"{}\"", _height.toString());
        output.push_back(std::move(buffer));
    }

    //  Output remaining attributes
    if (_attributes.size() > 0) {
        //  If all attributes become editable, this logic can
        //  be removed.
        hasAttributes = true;
        buffer = _attributes.write();
        output.push_back(std::move(buffer));
    }

    return hasAttributes;
}
