#include "SvgCommentElement.hpp"

//	Standard library
#include <format> //  std::format
#include <memory> //  std::make_unique

#include "utility_p.hpp" //  SvgRope

/*
From w3.org: https://www.w3.org/TR/SVG2/types.html#InterfaceSVGElement
    
All of the SVG DOM interfaces that correspond directly to elements in the
SVG language (such as the SVGPathElement interface for the ‘path’ element)
derive from the SVGElement interface.
*/

//	Public interface
SvgCommentElement::SvgCommentElement()
    : SvgElement()
{
    _elementType = SvgElement::SvgType::SvgCommentElement;
}

SvgCommentElement::SvgCommentElement(const std::string &comment)
    : SvgCommentElement()
{
    _value = comment;
}

void SvgCommentElement::toXml(SvgRope &output) const
{
    std::string buffer = std::format("<!--{}-->", _value);
    output.push_back(std::move(buffer));
    //  Comments cannot have children or attributes
}

std::unique_ptr<SvgElement> SvgCommentElement::clone() const
{
    return std::make_unique<SvgCommentElement>(*this);
}
