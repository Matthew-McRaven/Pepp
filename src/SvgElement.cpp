#include "SvgElement.hpp"
#include "SvgElement_p.hpp"

// SvgElement uses PIMPL pattern to manage data access. Data and functions
// in Impl struct are not part of the public interface and may
// change without notice.

//	Standard library
#include <memory>
#include <string>

/*
From w3.org: https://www.w3.org/TR/SVG2/types.html#InterfaceSVGElement
    
All of the SVG DOM interfaces that correspond directly to elements in the
SVG language (such as the SVGPathElement interface for the ‘path’ element)
derive from the SVGElement interface.
*/

//	Public interface
SvgElement::SvgElement(bool base)
{
    if (base)
        _impl = std::make_unique<SvgElementImpl>();
}

SvgElement::SvgElement(const std::string &xmlName, const std::string &value)
    : SvgElement()
{
    _impl->xmlName = xmlName;
    _impl->value = value;
}

//	Need to move implementation after Impl structure so unique_ptr will see full
//	definition. Otherwise, compiler error
SvgElement::~SvgElement() = default;
SvgElement::SvgElement(SvgElement &&) noexcept = default;
SvgElement &SvgElement::operator=(SvgElement &&) noexcept = default;

//	SvgElement appears in containers that require a copy constructor
//  Add copy logic for contains (e.g., list).
SvgElement::SvgElement(const SvgElement &rhs)
    : _impl(nullptr)
{
    if (rhs._impl)
        _impl = std::make_unique<SvgElementImpl>(*rhs._impl);
}
SvgElement &SvgElement::operator=(const SvgElement &rhs)
{
    if (!rhs._impl)
        _impl.reset();
    else if (!_impl)
        _impl = std::make_unique<SvgElementImpl>(*rhs._impl);
    else {
        *_impl = *rhs._impl;
    }

    return *this;
}

//  Generic Dom fields
SvgElement::SvgType SvgElement::elementType() const
{
    return _impl->elementType;
}
void SvgElement::setElementType(SvgElement::SvgType elementType)
{
    _impl->elementType = elementType;
}

std::string &SvgElement::xmlName() const
{
    return _impl->xmlName;
}
void SvgElement::setXmlName(std::string xmlName)
{
    _impl->xmlName = xmlName;
}
std::string &SvgElement::value() const
{
    return _impl->value;
}
void SvgElement::setValue(std::string value)
{
    _impl->value = value;
}

//  Svg specific fields
std::string &SvgElement::id() const
{
    return _impl->id;
}
void SvgElement::setId(std::string id)
{
    _impl->id = id;
}
std::string &SvgElement::className() const
{
    return _impl->className;
}
void SvgElement::setClassName(std::string className)
{
    _impl->className = className;
}
std::string &SvgElement::title() const
{
    return (_impl->title == nullptr) ? _impl->empty : _impl->title->xmlName();
}
void SvgElement::setTitle(std::string title)
{
    //  Add logic later to create element when missing
    if (_impl->title != nullptr) {
        _impl->title->setValue(title);
    }
}
std::string &SvgElement::metadata() const
{
    return (_impl->metadata == nullptr) ? _impl->empty : _impl->metadata->xmlName();
}
void SvgElement::setMetadata(std::string metadata)
{
    //  Add logic later to create element when missing
    if (_impl->metadata != nullptr) {
        _impl->metadata->setValue(metadata);
    }
}
std::string &SvgElement::desc() const
{
    return (_impl->desc == nullptr) ? _impl->empty : _impl->desc->xmlName();
}
void SvgElement::setDesc(std::string desc)
{
    //  Add logic later to create element when missing
    if (_impl->desc != nullptr) {
        _impl->desc->setValue(desc);
    }
}

//  Dimension accessors
auto SvgElement::x() const
{
    return _impl->x.value;
}
void SvgElement::setX(double x)
{
    _impl->x.value = x;
}
void SvgElement::setX(const std::string_view sv)
{
    _impl->x.fromString(sv);
}
auto SvgElement::y() const
{
    return _impl->y.value;
}
void SvgElement::setY(double y)
{
    _impl->y.value = y;
}
void SvgElement::setY(const std::string_view sv)
{
    _impl->y.fromString(sv);
}
auto SvgElement::width() const
{
    return _impl->width.value;
}
void SvgElement::setWidth(double width)
{
    _impl->width.value = std::max(width, 0.0);
}
void SvgElement::setWidth(const std::string_view sv)
{
    _impl->width.fromString(sv);
}
auto SvgElement::height() const
{
    return _impl->height;
}
void SvgElement::setHeight(double height)
{
    _impl->height.value = std::max(height, 0.0);
}
void SvgElement::setHeight(const std::string_view sv)
{
    _impl->height.fromString(sv);
}

//  Generic elements that capture unprocessed svg data
const XmlAttributes &SvgElement::attributes() const
{
    return _impl->attributes;
}
XmlAttributes &SvgElement::attributes()
{
    return _impl->attributes;
}

const std::list<SvgElement *> &SvgElement::children() const
{
    return _impl->elements;
}
std::list<SvgElement *> &SvgElement::children()
{
    return _impl->elements;
}

void SvgElement::appendChild(SvgElement *child)
{
    //  Certain data is contained in elements. Save
    //  pointer to allow future programitic updates.
    if (child->xmlName() == "desc"s) {
        _impl->desc = child;
    } else if (child->xmlName() == "metadata"s) {
        _impl->metadata = child;
    } else if (child->xmlName() == "title"s) {
        _impl->title = child;
    }
    //  All elements are saved, including special elements above.
    //  Used for persistence to Xml.
    _impl->elements.push_back(child);
}

void SvgElement::toXml(SvgRope &output) const
{
    //  Id currently has element name. Change when attributes are supported
    if (_impl->elementType == SvgElement::SvgType::DomComment) {
        std::string buffer = std::format("<!--{}-->", _impl->value);
        output.push_back(std::move(buffer));
        //  Comments cannot have children or attributes
        return;
    }
    output.push_back("<" + _impl->xmlName);

    _impl->attributeXml(output);

    //  No child elements and no values, add end tag
    if (_impl->elements.empty() && _impl->value.empty()) {
        output.push_back(" />");
        return;
    }
    output.push_back(">");

    //	Save value, if present
    if (!_impl->value.empty())
        output.push_back(_impl->value);

    for (const auto *element : _impl->elements) {
        element->toXml(output);
    }
    //  After child elements, add closing element
    output.push_back("</" + _impl->xmlName + ">");
}

bool SvgElement::setAttribute(const std::string &key, const std::string &value)
{
    return _impl->setAttribute(key, value);
}

bool SvgElementImpl::setAttribute(const std::string &key, const std::string &value)
{
    if (key == "id"s) {
        id = value;
        return true;
    }
    if (key == "x"s) {
        return x.fromString(value);
    }
    if (key == "y"s) {
        return y.fromString(value);
    }
    if (key == "width"s) {
        return width.fromString(value);
    }
    if (key == "height"s) {
        return height.fromString(value);
    }

    //  Cache attributes that are not manipulated above.
    attributes.add(key, value);
    return true;
}

bool SvgElementImpl::attributeXml(SvgRope &output) const
{
    std::string buffer;
    bool hasAttributes = false;
    if (!id.empty()) {
        hasAttributes = true;
        buffer = std::format(" id=\"{}\"", id);
        output.push_back(std::move(buffer));
    }
    if (!x.empty()) {
        hasAttributes = true;
        buffer = std::format(" x=\"{}\"", x.toString());
        output.push_back(std::move(buffer));
    }
    if (!y.empty()) {
        hasAttributes = true;
        buffer = std::format(" y=\"{}\"", y.toString());
        output.push_back(std::move(buffer));
    }
    if (!width.empty()) {
        hasAttributes = true;
        buffer = std::format(" width=\"{}\"", width.toString());
        output.push_back(std::move(buffer));
    }
    if (!height.empty()) {
        hasAttributes = true;
        buffer = std::format(" height=\"{}\"", height.toString());
        output.push_back(std::move(buffer));
    }

    //  Output remaining attributes
    if (attributes.size() > 0) {
        //  If all attributes become editable, this logic can
        //  be removed.
        hasAttributes = true;
        buffer = attributes.write();
        output.push_back(std::move(buffer));
    }

    return hasAttributes;
}
