#include "SvgSvgElement.hpp"
#include "SvgElement_p.hpp"
#include "SvgSvgElement_p.hpp"

// SvgElement uses PIMPL pattern to manage data access. Data and functions
// in Impl struct are not part of the public interface and may
// change without notice.

//	Standard library
#include <memory>
#include <string>

/*
From w3.org: https://www.w3.org/TR/SVG2/struct.html#InterfaceSVGSVGElement
    
An SVGSVGElement object represents an ‘svg’ element in the DOM. The SVGSVGElement
interface also contains miscellaneous utility methods, such as data type object
factory methods.
*/

//	Public interface
SvgSvgElement::SvgSvgElement()
    : SvgElement(false)
{
    //  Override base pimpl structure when overridding
    _impl.reset(static_cast<SvgElementImpl *>(new SvgSvgElementImpl()));

    _impl->elementType = SvgElement::SvgType::SvgSvgElement;
}

SvgSvgElement::SvgSvgElement(const std::string &id, const std::string &value)
    : SvgSvgElement()
{
    _impl->id = id;
    _impl->value = value;
}

//	Need to move implementation after Impl structure so unique_ptr will see full
//	definition. Otherwise, compiler error
SvgSvgElement::~SvgSvgElement() = default;
SvgSvgElement::SvgSvgElement(SvgSvgElement &&) noexcept = default;
SvgSvgElement &SvgSvgElement::operator=(SvgSvgElement &&) noexcept = default;

//	SvgElement appears in containers that require a copy constructor
//  Add copy logic for contains (e.g., list).
/*SvgSvgElement::SvgSvgElement(const SvgSvgElement &rhs)
{
    _impl.reset();

    if (rhs._impl)
        _impl = std::make_unique<SvgSvgElementImpl>(*rhs._impl);
}
SvgSvgElement &SvgSvgElement::operator=(const SvgSvgElement &rhs)
{
    if (!rhs._impl)
        _impl.reset();
    else if (!_impl)
        _impl = std::make_unique<SvgSvgElementImpl>(*rhs._impl);
    else {
        *_impl = *rhs._impl;
    }

    return *this;
}*/

/*void SvgSvgElement::toXml(std::list<std::string> &output) const
{
    //  Id currently has element name. Change when attributes are supported
    //std::string buffer;
    output.push_back("<" + _impl->id);
    if (_impl->attributes().size() > 0) {
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
}*/
