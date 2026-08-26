#include "SvgCommentElement.hpp"
#include "SvgDocument_p.hpp"

//	Standard library
#include <format> //  std::format
#include <string>

#include "SvgDocument_p.hpp"
#include "utility_p.hpp"

/*
From w3.org: https://www.w3.org/TR/SVG2/types.html#InterfaceSVGElement
    
All of the SVG DOM interfaces that correspond directly to elements in the
SVG language (such as the SVGPathElement interface for the ‘path’ element)
derive from the SVGElement interface.
*/

//	Public interface
SvgCommentElement::SvgCommentElement()
    : SvgInterface()
{
    _elementType = SvgInterface::SvgType::SvgCommentElement;
}

SvgCommentElement::SvgCommentElement(DocumentImpl *d)
    : SvgCommentElement()
{
    _doc = d;
}

SvgCommentElement::SvgCommentElement(const std::string &comment)
    : SvgCommentElement()
{
    _comment = comment;
}

//  Generic Dom fields
const std::string &SvgCommentElement::comment() const
{
    return _comment;
}
void SvgCommentElement::setComment(std::string value)
{
    _comment = value;
}

void SvgCommentElement::toXml(SvgRope &output) const
{
    std::string buffer = std::format("<!--{}-->", _comment);
    output.push_back(std::move(buffer));
    //  Comments cannot have children or attributes
}

SvgInterface::SvgType SvgCommentElement::elementType() const
{
    return _elementType;
}

void SvgCommentElement::setElementType(SvgInterface::SvgType elementType)
{
    _elementType = elementType;
}
