#include "SvgCDataElement.hpp"

//	Standard library
#include <format> //  std::format
#include <memory> //  std::make_unique

#include "utility_p.hpp" //  SvgRope

/*
From w3.org: https://www.w3.org/TR/SVG2/interact.html#ScriptElement. 
    
Technically, CDATA is a child element under the ScriptElement above.
The output format is special, and this class handles the parsing
and output.
*/

//	Public interface
SvgCDataElement::SvgCDataElement()
    : SvgElement()
{
    _elementType = SvgElement::SvgType::SvgCDataElement;
}

SvgCDataElement::SvgCDataElement(const std::string &cdata)
    : SvgCDataElement()
{
    _value = cdata;
}

void SvgCDataElement::toXml(SvgRope &output) const
{
    std::string buffer = std::format("<![CDATA[{}]]>", _value);
    output.push_back(std::move(buffer));
    //  CData cannot have children or attributes
}

std::unique_ptr<SvgElement> SvgCDataElement::clone() const
{
    return std::make_unique<SvgCDataElement>(*this);
}
