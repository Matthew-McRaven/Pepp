#include "SvgElement.hpp"

//	Standard library
#include <algorithm> //  std::transform
#include <memory> //  std::make_unique

#include "SvgCDataElement.hpp"
#include "SvgCommentElement.hpp"
#include "SvgDocument.hpp"
#include "SvgFactory.hpp"
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

SvgElement::SvgElement(const SvgElement &original)
    : SvgElement()
{
    _doc = SvgInterface::currentDocument();

    _elementType = original._elementType;
    _xmlName = original._xmlName;
    _value = original._value;

    setId(original._id);
    _className = original._className;

    _x = original._x;
    _y = original._y;
    _width = original._width;
    _height = original._height;

    //  Use class logic for attributes
    _attributes = original._attributes;

    for (const auto &element : original.children()) {
        _children.push_back(element->clone());
        auto copy = static_cast<SvgElement *>(_children.back().get());

        switch (copy->elementType()) {
        case SvgType::Type::SvgDescElement:
            this->_desc = copy;
            break;
        case SvgType::Type::SvgMetadataElement:
            this->_metadata = copy;
            break;
        case SvgType::Type::SvgTitleElement:
            this->_title = copy;
            break;
        default:
            break;
        }
    }
}

//  Generic Dom fields
SvgType::Type SvgElement::elementType() const
{
    return _elementType;
}
void SvgElement::setElementType(const SvgType::Type elementType)
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
    if (_doc && !id.empty())
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
SvgUnitValue SvgElement::x() const
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
SvgUnitValue SvgElement::y() const
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
SvgUnitValue SvgElement::width() const
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
SvgUnitValue SvgElement::height() const
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

const std::list<std::unique_ptr<SvgInterface>> &SvgElement::children() const
{
    return _children;
}
std::list<std::unique_ptr<SvgInterface>> &SvgElement::children()
{
    return _children;
}

SvgInterface *SvgElement::createElement(const std::string &name)
{
    if (name == "comment"s)
        createElement(SvgType::Type::SvgCommentElement);
    else if (name == "cdata"s)
        createElement(SvgType::Type::SvgCDataElement);
    else if (name == "rect"s)
        createElement(SvgType::Type::SvgRectElement);
    else if (name == "defs"s)
        createElement(SvgType::Type::SvgDefsElement);
    else if (name == "g"s)
        createElement(SvgType::Type::SvgGElement);
    else if (name == "desc"s)
        createElement(SvgType::Type::SvgDescElement);
    else if (name == "metadata"s)
        createElement(SvgType::Type::SvgMetadataElement);
    else if (name == "title"s)
        createElement(SvgType::Type::SvgTitleElement);
    else {
        createElement(SvgType::Type::SvgUnknownElement);
    }

    SvgElement *element = static_cast<SvgElement *>(_children.back().get());
    element->setXmlName(name);

    return _children.back().get();
}

SvgInterface *SvgElement::createElement(const SvgType::Type type)
{
    _children.push_back(SvgFactory::createElement(type));

    //  Some elements require additional processing
    auto child = static_cast<SvgElement *>(_children.back().get());
    if (child->elementType() == SvgType::Type::SvgUnknownElement)
        child->setElementType(type);

    //  We track certain types, add pointers now
    switch (type) {
    case SvgType::Type::SvgDescElement:
        _desc = child;
        break;
    case SvgType::Type::SvgMetadataElement:
        _metadata = child;
        break;
    case SvgType::Type::SvgTitleElement: {
        _title = child;
        break;
    }
    }

    //  All children must refer to this document
    child->setCurrentDocument(_doc);
    return child;
}

void SvgElement::appendChild(SvgInterface *child)
{
    // This is a move operation. Add move logic later.
    //  Just return now.
    if (child->currentDocument() == _doc)
        return;

    _children.emplace_back(child->clone());
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
        buffer = fmt::format(" id=\"{}\"", _id);
        output.push_back(std::move(buffer));
    }
    if (!_x.empty()) {
        hasAttributes = true;
        buffer = fmt::format(" x=\"{}\"", _x.toString());
        output.push_back(std::move(buffer));
    }
    if (!_y.empty()) {
        hasAttributes = true;
        buffer = fmt::format(" y=\"{}\"", _y.toString());
        output.push_back(std::move(buffer));
    }
    if (!_width.empty()) {
        hasAttributes = true;
        buffer = fmt::format(" width=\"{}\"", _width.toString());
        output.push_back(std::move(buffer));
    }
    if (!_height.empty()) {
        hasAttributes = true;
        buffer = fmt::format(" height=\"{}\"", _height.toString());
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
