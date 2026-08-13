#include "SvgRectElement.hpp"
#include "SvgElement_p.hpp"
#include "SvgRectElement_p.hpp"

// SvgElement uses PIMPL pattern to manage data access. Data and functions
// in Impl struct are not part of the public interface and may
// change without notice.

//	Standard library
#include <memory>
#include <string>
using namespace std::string_literals;

/*
From w3.org: https://www.w3.org/TR/SVG2/shapes.html#InterfaceSVGRectElement
    
An SvgRectElement object represents an ‘rect’ element in the DOM. The SvgRectElement
interface also contains miscellaneous utility methods, such as data type object
factory methods.
*/

//	Public interface
SvgRectElement::SvgRectElement()
    : SvgElement(false)
{
    //  Override base pimpl structure when overridding interface
    //  _impl is memory managed
    _impl.reset(static_cast<SvgElementImpl *>(new SvgRectElementImpl()));

    _impl->elementType = SvgElement::SvgType::SvgRectElement;
}

SvgRectElement::SvgRectElement(const std::string &xmlName, const std::string &value)
    : SvgRectElement()
{
    _impl->xmlName = xmlName;
    _impl->value = value;
}

//	Need to move implementation after Impl structure so unique_ptr will see full
//	definition. Otherwise, compiler error
SvgRectElement::~SvgRectElement() = default;
SvgRectElement::SvgRectElement(SvgRectElement &&) noexcept = default;
SvgRectElement &SvgRectElement::operator=(SvgRectElement &&) noexcept = default;

//  Cast base pointer to derived class. Need for all custom functions in derived class
SvgRectElementImpl *SvgRectElement::derivedThis()
{
    static SvgRectElementImpl *derived{};
    if (derived == nullptr) {
        derived = static_cast<SvgRectElementImpl *>(_impl.get());
    }
    return derived;
}

const SvgRectElementImpl *SvgRectElement::derivedThis() const
{
    static SvgRectElementImpl *derived{};
    if (derived == nullptr) {
        derived = static_cast<SvgRectElementImpl *>(_impl.get());
    }
    return derived;
}

//  Derived class accessors
//  Dimension accessors
auto SvgRectElement::rx() const
{
    return derivedThis()->rx.value;
}
void SvgRectElement::setRX(double x)
{
    derivedThis()->rx.value = x;
}
void SvgRectElement::setRX(const std::string_view sv)
{
    derivedThis()->rx.fromString(sv);
}
auto SvgRectElement::ry() const
{
    return derivedThis()->ry.value;
}
void SvgRectElement::setRY(double y)
{
    derivedThis()->ry.value = y;
}
void SvgRectElement::setRY(const std::string_view sv)
{
    derivedThis()->ry.fromString(sv);
}

void SvgRectElement::toXml(std::list<std::string> &output) const
{
    //  Hard coded attribute. Spelling is case specific.
    output.push_back("<rect");

    //  Output changeable headers
    _impl->attributeXml(output);

    //  No child elements, and no values, add end tag
    if (_impl->elements.empty() && _impl->value.empty()) {
        output.push_back(" />");
        return;
    }
    output.push_back(">");

    for (const auto *element : _impl->elements) {
        element->toXml(output);
    }
    //  When child elements, add closing element
    output.push_back("</rect>");
}

bool SvgRectElement::setAttribute(const std::string &key, const std::string &value)
{
    return _impl->setAttribute(key, value);
}

bool SvgRectElementImpl::setAttribute(const std::string &key, const std::string &value)
{
    if (key == "rx"s) {
        return rx.fromString(value);
    }
    if (key == "ry"s) {
        return ry.fromString(value);
    }

    //  Let base class handle remaining elements
    return SvgElementImpl::setAttribute(key, value);
}

bool SvgRectElementImpl::attributeXml(std::list<std::string> &output) const
{
    //  Get parent attributes first
    bool hasAttributes = SvgElementImpl::attributeXml(output);

    //  Go through derived class elements
    if (!rx.empty()) {
        hasAttributes = true;
        std::string buffer = std::format(" rx=\"{}\"", rx.toString());
        output.push_back(std::move(buffer));
    }
    if (!ry.empty()) {
        hasAttributes = true;
        std::string buffer = std::format(" ry=\"{}\"", ry.toString());
        output.push_back(std::move(buffer));
    }

    return hasAttributes;
}