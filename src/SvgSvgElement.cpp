#include "SvgSvgElement.hpp"

//	Standard library
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
    : Cloneable<SvgSvgElement, SvgElement>()
{
    SvgElement::setElementType(SvgInterface::SvgType::SvgSvgElement);
}

SvgSvgElement::SvgSvgElement(const std::string &xmlName, const std::string &value)
    : SvgSvgElement()
{
    SvgElement::setXmlName(xmlName);
    SvgElement::setValue(value);
}

//  Derived class accessors
SvgRect &SvgSvgElement::viewBox()
{
    return _viewBox;
}
const SvgRect &SvgSvgElement::viewBox() const
{
    return _viewBox;
}

void SvgSvgElement::toXml(SvgRope &output) const
{
    //  Hard coded attribute. Spelling is case specific.
    output.push_back("<svg");

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
    output.push_back("</svg>");
}

bool SvgSvgElement::setAttribute(const std::string &key, const std::string &value)
{
    if (key == "viewBox"s) {
        return _viewBox.fromString(value);
    }

    //  Let base class handle remaining elements
    return SvgElement::setAttribute(key, value);
}

bool SvgSvgElement::attributeXml(SvgRope &output) const
{
    //  Get parent attributes first
    bool hasAttributes = SvgElement::attributeXml(output);
    if (!_viewBox.empty()) {
        hasAttributes = true;
        std::string buffer = std::format(" viewBox=\"{}\"", _viewBox.toString());
        output.push_back(std::move(buffer));
    }

    return hasAttributes;
}