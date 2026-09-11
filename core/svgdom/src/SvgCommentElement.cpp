#include "SvgCommentElement.hpp"

//	Standard library
#include <format> //  std::format

#include "utility_p.hpp" //  SvgRope

/*
From w3.org: https://www.w3.org/TR/SVG2/types.html#InterfaceSVGElement
    
All of the SVG DOM interfaces that correspond directly to elements in the
SVG language (such as the SVGPathElement interface for the ‘path’ element)
derive from the SVGElement interface.
*/

//	Public interface
SvgCommentElement::SvgCommentElement()
    : Cloneable<SvgCommentElement, SvgElement>()
{
    SvgElement::setElementType(SvgType::Type::SvgCommentElement);
}

SvgCommentElement::SvgCommentElement(const std::string &comment)
    : SvgCommentElement()
{
    SvgElement::setValue(comment);
}

bool SvgCommentElement::setAttribute(const std::string &key, const std::string &value)
{
    //  Comments cannot have children or attributes
    return true;
}

void SvgCommentElement::toXml(SvgRope &output) const
{
    std::string buffer = std::format("<!--{}-->", SvgElement::value());
    output.push_back(std::move(buffer));
    //  Comments cannot have children or attributes
}
