#include "SvgUseElement.hpp"

//	Standard library
#include <string>
using namespace std::string_literals;

/*
From w3.org: https://www.w3.org/TR/SVG2/shapes.html#InterfaceSvgUseElement
    
The <use> element takes nodes from within an SVG document, and duplicates them
somewhere else. The effect is the same as if the nodes were deeply cloned into
a non-exposed DOM, then pasted where the <use> element is, much like cloned
<template> elements.
*/

//	Public interface
SvgUseElement::SvgUseElement()
    : Cloneable<SvgUseElement, SvgElement>()
{
    SvgElement::setElementType(SvgType::Type::SvgUseElement);
}

SvgUseElement::SvgUseElement(const std::string &xmlName, const std::string &value)
    : SvgUseElement()
{
    SvgElement::setXmlName(xmlName);
    SvgElement::setValue(value);
}

//  Derived class accessors
//  Dimension accessors
auto SvgUseElement::href() const
{
    return _href;
}
void SvgUseElement::setHref(const std::string &href)
{
    //  Link must include # sign to be valid.
    //  Add if missing
    if (href.front() != '#') {
        _href = "#" + href;
        return;
    }

    _href = href;
}

void SvgUseElement::toXml(SvgRope &output) const
{
    //  Hard coded attribute. Spelling is case specific.
    output.push_back("<use");

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
    output.push_back("</use>");
}

bool SvgUseElement::setAttribute(const std::string &key, const std::string &value)
{
    if (key == "href"s) {
        setHref(value);
        return true;
    }

    //  Let base class handle remaining elements
    return SvgElement::setAttribute(key, value);
}

bool SvgUseElement::attributeXml(SvgRope &output) const
{
    bool hasAttributes = false;

    //  Go through derived class elements
    if (!_href.empty()) {
        hasAttributes = true;
        std::string buffer = fmt::format(" href=\"{}\"", _href);
        output.push_back(std::move(buffer));
    }

    //  Get parent attributes last.
    hasAttributes |= SvgElement::attributeXml(output);

    return hasAttributes;
}