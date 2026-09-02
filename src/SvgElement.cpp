#include "SvgElement.hpp"

//	Standard library
#include <memory> //  std::make_unique

#include "SvgCDataElement.hpp"
#include "SvgCommentElement.hpp"
#include "SvgDocument.hpp"
#include "SvgRectElement.hpp"

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

SvgElement::SvgElement(const SvgElement &orig)
    : SvgElement()
{
    _elementType = orig._elementType;
    _xmlName = orig._xmlName;
    _value = orig._value;

    _id = orig._id;
    _className = orig._className;

    _x = orig._x;
    _y = orig._y;
    _width = orig._width;
    _height = orig._height;

    _attributes = orig._attributes;
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

void SvgElement::setDocument(Document *doc)
{
    _doc = doc;
}

//  Svg specific fields
const std::string &SvgElement::id() const
{
    return _id;
}
void SvgElement::setId(const std::string &id)
{
    //  Make sure pointer is valid
    if (_doc)
        _doc->addElementId(id, this);
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
    //  Create element if missing
    if (_title == nullptr) {
        createElement("title"s);
    }
    _title->setValue(title);
}
const std::string &SvgElement::metadata() const
{
    return (_metadata == nullptr) ? _empty : _metadata->xmlName();
}
void SvgElement::setMetadata(const std::string &metadata)
{
    //  Create element if missing
    if (_metadata == nullptr) {
        createElement("metadata"s);
    }
    _metadata->setValue(metadata);
}
const std::string &SvgElement::desc() const
{
    return (_desc == nullptr) ? _empty : _desc->xmlName();
}
void SvgElement::setDesc(const std::string &desc)
{
    //  Create element if missing
    if (_desc == nullptr) {
        createElement("desc"s);
    }
    _desc->setValue(desc);
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

const auto &SvgElement::children() const
{
    return _children;
}
auto &SvgElement::children()
{
    return _children;
}

SvgElement *SvgElement::createElement(const std::string &name)
{
    if (name == "comment"s)
        createElement(SvgType::SvgCommentElement);
    else if (name == "cdata"s)
        createElement(SvgType::SvgCDataElement);
    else if (name == "rect"s)
        createElement(SvgType::SvgRectElement);
    else if (name == "defs"s)
        createElement(SvgType::SvgDefsElement);
    else if (name == "g"s)
        createElement(SvgType::SvgGElement);
    else if (name == "desc"s)
        createElement(SvgType::SvgDescElement);
    else if (name == "metadata"s)
        createElement(SvgType::SvgMetadataElement);
    else if (name == "title"s)
        createElement(SvgType::SvgTitleElement);
    else {
        createElement(SvgType::SvgUnknownElement);
    }

    auto element = _children.back().get();
    element->setXmlName(name);

    return element;
}

SvgElement *SvgElement::createElement(const SvgType type)
{
    switch (type) {
    case SvgType::SvgCommentElement:
        _children.push_back(std::make_unique<SvgCommentElement>());
        break;
    case SvgType::SvgCDataElement:
        _children.push_back(std::make_unique<SvgCDataElement>());
        break;
    case SvgType::SvgRectElement:
        _children.push_back(std::make_unique<SvgRectElement>());
        break;
    case SvgType::SvgDefsElement:
        _children.push_back(std::make_unique<SvgElement>("defs"s));
        _children.back().get()->setElementType(type);
        break;
    case SvgType::SvgGElement:
        _children.push_back(std::make_unique<SvgElement>("g"s));
        _children.back().get()->setElementType(type);
        break;
    case SvgType::SvgDescElement:
        _children.push_back(std::make_unique<SvgElement>("desc"s));
        _desc = _children.back().get();
        _desc->setElementType(type);
        break;
    case SvgType::SvgMetadataElement:
        _children.push_back(std::make_unique<SvgElement>("metadata"s));
        _metadata = _children.back().get();
        _metadata->setElementType(type);
        break;
    case SvgType::SvgTitleElement:
        _children.push_back(std::make_unique<SvgElement>("title"s));
        _title = _children.back().get();
        _title->setElementType(type);
        break;
    default:
        _children.push_back(std::make_unique<SvgElement>());
    }

    return _children.back().get();
}

void SvgElement::toXml(SvgRope &output) const
{
    output.push_back("<" + _xmlName);

    attributeXml(output);

    //  No child elements and no values, add end tag
    if (_children.empty() && _value.empty()) {
        output.push_back(" />");
        return;
    }
    output.push_back(">");

    //	Save value, if present
    if (!_value.empty())
        //  Pass copy
        output.push_back(std::string(_value));

    for (const auto &element : _children) {
        element->toXml(output);
    }
    //  After child elements, add closing element
    output.push_back("</" + _xmlName + ">");
}

bool SvgElement::setAttribute(const std::string &key, const std::string &value)
{
    if (key == "id"s) {
        setId(value);
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

std::unique_ptr<SvgElement> SvgElement::clone() const
{
    auto copy = std::make_unique<SvgElement>(*this);

    for (const auto &child : _children) {
        copy->_children.push_back(child->clone());
    }

    return copy;
}
