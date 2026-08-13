#include "SvgSvgElement.hpp"
#include "SvgElement_p.hpp"
#include "SvgSvgElement_p.hpp"

// SvgElement uses PIMPL pattern to manage data access. Data and functions
// in Impl struct are not part of the public interface and may
// change without notice.

//	Standard library
#include <memory>
#include <string>
using namespace std::string_literals;

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
    //  Override base pimpl structure when overridding interface
    //  _impl is memory managed
    _impl.reset(static_cast<SvgElementImpl *>(new SvgSvgElementImpl()));

    _impl->elementType = SvgElement::SvgType::SvgSvgElement;
}

SvgSvgElement::SvgSvgElement(const std::string &xmlName, const std::string &value)
    : SvgSvgElement()
{
    _impl->xmlName = xmlName;
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

//  Cast base pointer to derived class. Need for all custom functions in derived class
SvgSvgElementImpl *SvgSvgElement::derivedThis()
{
    static SvgSvgElementImpl *derived{};
    if (derived == nullptr) {
        derived = static_cast<SvgSvgElementImpl *>(_impl.get());
    }
    return derived;
}

const SvgSvgElementImpl *SvgSvgElement::derivedThis() const
{
    static SvgSvgElementImpl *derived{};
    if (derived == nullptr) {
        derived = static_cast<SvgSvgElementImpl *>(_impl.get());
    }
    return derived;
}

//  Derived class accessors
SvgRect &SvgSvgElement::viewBox()
{
    return derivedThis()->viewBox;
}
const SvgRect &SvgSvgElement::viewBox() const
{
    return derivedThis()->viewBox;
}

void SvgSvgElement::toXml(std::list<std::string> &output) const
{
    //  Hard coded attribute. Spelling is case specific.
    output.push_back("<svg");

    //  Output changeable headers
    auto actualData = derivedThis();
    bool customAttrData = false;

    if (actualData && !actualData->viewBox.empty()) {
        customAttrData = true;
        std::string buffer = std::format(" viewBox=\"{}\"", actualData->viewBox.toString());
        output.push_back(std::move(buffer));
    }

    if (!_impl->x.empty()) {
        customAttrData = true;
        std::string buffer = std::format(" x=\"{}\"", _impl->x.toString());
        output.push_back(std::move(buffer));
    }
    if (!_impl->y.empty()) {
        customAttrData = true;
        std::string buffer = std::format(" y=\"{}\"", _impl->y.toString());
        output.push_back(std::move(buffer));
    }
    if (!_impl->width.empty()) {
        customAttrData = true;
        std::string buffer = std::format(" width=\"{}\"", _impl->width.toString());
        output.push_back(std::move(buffer));
    }
    if (!_impl->height.empty()) {
        customAttrData = true;
        std::string buffer = std::format(" height=\"{}\"", _impl->height.toString());
        output.push_back(std::move(buffer));
    }

    if (_impl->attributes.size() > 0) {
        //  Output remaining attributes
        //  If all attributes become editable, this logic can
        //  be removed.
        output.push_back(_impl->attributes.write());
    }

    //  No child elements and no values, add end tag
    if (_impl->elements.empty() && _impl->value.empty() && !customAttrData) {
        output.push_back(" />");
        return;
    }
    output.push_back(">");

    for (const auto *element : _impl->elements) {
        element->toXml(output);
    }
    //  When child elements, add closing element
    output.push_back("</svg>");
}

bool SvgSvgElement::setAttribute(const std::string &key, const std::string &value)
{
    return _impl->setAttribute(key, value);
}

bool SvgSvgElementImpl::setAttribute(const std::string &key, const std::string &value)
{
    if (key == "viewBox"s) {
        return viewBox.fromString(value);
    }

    //  Let base class handle remaining elements
    return SvgElementImpl::setAttribute(key, value);
}