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
SvgElement::SvgElement()
    : _impl(std::make_unique<SvgElementImpl>())
{}

SvgElement::SvgElement(const std::string &id, const std::string &value)
    : SvgElement()
{
    _impl->id = id;
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

std::string SvgElement::id() const
{
    return _impl->id;
}
void SvgElement::setId(std::string id)
{
    _impl->id = id;
}

std::string SvgElement::className() const
{
    return _impl->className;
}
void SvgElement::setClassName(std::string className)
{
    _impl->className = className;
}
std::string SvgElement::value() const
{
    return _impl->value;
}
void SvgElement::setValue(std::string value)
{
    _impl->value = value;
}

auto SvgElement::attributes() const
{
    return _impl->attributes;
}
auto SvgElement::attributes()
{
    return _impl->attributes;
}

auto SvgElement::children() const
{
    return _impl->elements;
}
auto SvgElement::children()
{
    return _impl->elements;
}

void SvgElement::appendChild(SvgElement *child)
{
    _impl->elements.push_back(child);
}

void SvgElement::toXml(std::list<std::string> &output) const
{
    //  Id currently has element name. Change when attributes are supported
    //std::string buffer;
    output.push_back("<" + _impl->id);
    if (attributes().size() > 0) {
        // Add persistence logic here
    }

    //  No child elements and no values, add end tag
    if (_impl->elements.empty() && _impl->value.empty()) {
        output.push_back(" />");
        return;
    }
    output.push_back(">");

    for (const auto *element : _impl->elements) {
        element->toXml(output);
    }
    //  When child elements, add closing element
    output.push_back("</" + _impl->id + ">");
}
