#include "SvgRectElement.hpp"

//	Standard library
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
    : Cloneable<SvgRectElement, SvgElement>()
{
    SvgElement::setElementType(SvgType::Type::SvgCommentElement);
}

SvgRectElement::SvgRectElement(const std::string &xmlName, const std::string &value)
    : SvgRectElement()
{
    SvgElement::setXmlName(xmlName);
    SvgElement::setValue(value);
}

//  Derived class accessors
//  Dimension accessors
auto SvgRectElement::rx() const
{
    return _rx.value;
}
void SvgRectElement::setRx(double x)
{
    _rx.value = x;
}
void SvgRectElement::setRx(const std::string_view sv)
{
    _rx.fromString(sv);
}
auto SvgRectElement::ry() const
{
    return _ry.value;
}
void SvgRectElement::setRy(double y)
{
    _ry.value = y;
}
void SvgRectElement::setRy(const std::string_view sv)
{
    _ry.fromString(sv);
}

void SvgRectElement::toXml(SvgRope &output) const
{
    //  Hard coded attribute. Spelling is case specific.
    output.push_back("<rect");

    //  Output changeable headers
    attributeXml(output);

    //  No child elements, and no values, add end tag
    if (SvgElement::children().empty() && SvgElement::value().empty()) {
        output.push_back(" />");
        return;
    }
    output.push_back(">");

    for (const auto &element : SvgElement::children()) {
        element->toXml(output);
    }
    //  When child elements, add closing element
    output.push_back("</rect>");
}

bool SvgRectElement::setAttribute(const std::string &key, const std::string &value)
{
    if (key == "rx"s) {
        return _rx.fromString(value);
    }
    if (key == "ry"s) {
        return _ry.fromString(value);
    }

    //  Let base class handle remaining elements
    return SvgElement::setAttribute(key, value);
}

bool SvgRectElement::attributeXml(SvgRope &output) const
{
    //  Get parent attributes first
    bool hasAttributes = SvgElement::attributeXml(output);

    //  Go through derived class elements
    if (!_rx.empty()) {
        hasAttributes = true;
        std::string buffer = fmt::format(" rx=\"{}\"", _rx.toString());
        output.push_back(std::move(buffer));
    }
    if (!_ry.empty()) {
        hasAttributes = true;
        std::string buffer = fmt::format(" ry=\"{}\"", _ry.toString());
        output.push_back(std::move(buffer));
    }

    return hasAttributes;
}