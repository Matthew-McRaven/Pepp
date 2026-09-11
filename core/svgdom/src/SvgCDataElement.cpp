#include "SvgCDataElement.hpp"

//	Standard library
#include <fmt/format.h> //  fmt::format

#include "utility_p.hpp" //  SvgRope

/*
From w3.org: https://www.w3.org/TR/SVG2/interact.html#ScriptElement. 
    
Technically, CDATA is a child element under the ScriptElement above.
The output format is special, and this class handles the parsing
and output.
*/

//	Public interface
SvgCDataElement::SvgCDataElement()
    : Cloneable<SvgCDataElement, SvgElement>()
{
    SvgElement::setElementType(SvgType::Type::SvgCommentElement);
}

SvgCDataElement::SvgCDataElement(const std::string &cdata)
    : SvgCDataElement()
{
    SvgElement::setValue(cdata);
}

bool SvgCDataElement::setAttribute(const std::string &key, const std::string &value)
{
    //  CData cannot have children or attributes
    return true;
}

void SvgCDataElement::toXml(SvgRope &output) const
{
    std::string buffer = fmt::format("<![CDATA[{}]]>", SvgElement::value());
    output.push_back(std::move(buffer));
    //  CData cannot have children or attributes
}
