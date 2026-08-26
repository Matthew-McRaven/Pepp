#include "SvgBasicElement.hpp"
#include "SvgBasicElement_p.hpp"

// SvgElement uses PIMPL pattern to manage data access. Data and functions
// in Impl struct are not part of the public interface and may
// change without notice.

//	Standard library
#include <memory>
#include <string>

#include "SvgDocument_p.hpp"
#include "SvgInterface.h"
#include "utility_p.hpp"

/*
From w3.org: https://www.w3.org/TR/SVG2/types.html#InterfaceSVGElement
    
All of the SVG DOM interfaces that correspond directly to elements in the
SVG language (such as the SVGPathElement interface for the ‘path’ element)
derive from the SVGElement interface.
*/

//	Public interface
SvgBasicElement::SvgBasicElement()
    : SvgInterface()
{
    _impl = std::make_unique<SvgBasicElementImpl>();
    _impl->elementType = SvgInterface::SvgType::SvgBasicElement;
}

//	Public interface
SvgBasicElement::SvgBasicElement(DocumentImpl *d)
    : SvgBasicElement()
{
    _impl->doc = d;
}

SvgBasicElement::SvgBasicElement(const std::string &xmlName, const std::string &value)
    : SvgBasicElement()
{
    _impl->xmlName = xmlName;
    _impl->value = value;
}

//	Need to move implementation after Impl structure so unique_ptr will see full
//	definition. Otherwise, compiler error
SvgBasicElement::~SvgBasicElement() = default;
SvgBasicElement::SvgBasicElement(SvgBasicElement &&) noexcept = default;
SvgBasicElement &SvgBasicElement::operator=(SvgBasicElement &&) noexcept = default;

//	SvgElement appears in containers that require a copy constructor
//  Add copy logic for contains (e.g., list).
SvgBasicElement::SvgBasicElement(const SvgBasicElement &rhs)
    : _impl(nullptr)
{
    if (rhs._impl)
        _impl = std::make_unique<SvgBasicElementImpl>(*rhs._impl);
}
SvgBasicElement &SvgBasicElement::operator=(const SvgBasicElement &rhs)
{
    if (!rhs._impl)
        _impl.reset();
    else if (!_impl)
        _impl = std::make_unique<SvgBasicElementImpl>(*rhs._impl);
    else {
        *_impl = *rhs._impl;
    }

    return *this;
}

//  Generic Dom fields
std::string &SvgBasicElement::xmlName() const
{
    return _impl->xmlName;
}
void SvgBasicElement::setXmlName(std::string xmlName)
{
    _impl->xmlName = xmlName;
}
std::string &SvgBasicElement::value() const
{
    return _impl->value;
}
void SvgBasicElement::setValue(std::string value)
{
    _impl->value = value;
}

void SvgBasicElement::toXml(SvgRope &output) const
{
    //  Id currently has element name. Change when attributes are supported
    /*if (_impl->elementType == SvgInterface::SvgType::SvgComment) {
        std::string buffer = std::format("<!--{}-->", _impl->value);
        output.push_back(std::move(buffer));
        //  Comments cannot have children or attributes
        return;
    }*/
    output.push_back("<" + _impl->xmlName);

    //  no value, add end tag
    if (_impl->value.empty()) {
        output.push_back(" />");
        return;
    }
    output.push_back(">");

    //	Save value, if present
    if (!_impl->value.empty())
        output.push_back(_impl->value);

    //  After child elements, add closing element
    output.push_back("</" + _impl->xmlName + ">");
}

SvgInterface::SvgType SvgBasicElement::elementType() const
{
    return _impl->elementType;
}
void SvgBasicElement::setElementType(SvgInterface::SvgType elementType)
{
    _impl->elementType = elementType;
}
