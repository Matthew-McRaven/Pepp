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
SvgRectElement::SvgRectElement() : Cloneable<SvgRectElement, SvgElement>() {}

SvgRectElement::SvgRectElement(const std::string &xmlName) : SvgRectElement() {
  //  Value is not valid for rectangle element
  SvgElement::setXmlName(xmlName);
}

//  Derived class accessors
SvgType::Type SvgRectElement::elementType() const { return TYPE; }

//  Value is not valid for a rectangle
const std::string &SvgRectElement::value() const {
  static std::string empty{};
  return empty;
};
void SvgRectElement::setValue(const std::string &value) {};

//  Dimension accessors
SvgUnitValue SvgRectElement::rx() const { return _rx; }
void SvgRectElement::setRx(double x)
{
    _rx.value = x;
}
void SvgRectElement::setRx(const std::string_view sv)
{
    _rx.fromString(sv);
}
SvgUnitValue SvgRectElement::ry() const { return _ry; }
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

    //  No child elements, add end tag
    //  Value is not valid for rectangle element
    if (SvgElement::children().empty() /*&& SvgElement::value().empty()*/) {
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