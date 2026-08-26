#include "SvgCommentElement.hpp"
#include "SvgCommentElement_p.hpp"
#include "SvgDocument_p.hpp"

// SvgElement uses PIMPL pattern to manage data access. Data and functions
// in Impl struct are not part of the public interface and may
// change without notice.

//	Standard library
#include <format> //  std::format
#include <memory> //  std::make_unique
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
    _impl = std::make_unique<SvgCommentElementImpl>();
    _impl->elementType = SvgInterface::SvgType::SvgCommentElement;
}

SvgCommentElement::SvgCommentElement(DocumentImpl *d)
    : SvgCommentElement()
{
    _impl->doc = d;
}

SvgCommentElement::SvgCommentElement(const std::string &comment)
    : SvgCommentElement()
{
    _impl->comment = comment;
}

//	Need to move implementation after Impl structure so unique_ptr will see full
//	definition. Otherwise, compiler error
SvgCommentElement::~SvgCommentElement() = default;
SvgCommentElement::SvgCommentElement(SvgCommentElement &&) noexcept = default;
SvgCommentElement &SvgCommentElement::operator=(SvgCommentElement &&) noexcept = default;

//	SvgElement appears in containers that require a copy constructor
//  Add copy logic for contains (e.g., list).
SvgCommentElement::SvgCommentElement(const SvgCommentElement &rhs)
    : _impl(nullptr)
{
    if (rhs._impl)
        _impl = std::make_unique<SvgCommentElementImpl>(*rhs._impl);
}
SvgCommentElement &SvgCommentElement::operator=(const SvgCommentElement &rhs)
{
    if (!rhs._impl)
        _impl.reset();
    else if (!_impl)
        _impl = std::make_unique<SvgCommentElementImpl>(*rhs._impl);
    else {
        *_impl = *rhs._impl;
    }

    return *this;
}

//  Generic Dom fields
std::string &SvgCommentElement::comment() const
{
    return _impl->comment;
}
void SvgCommentElement::setComment(std::string value)
{
    _impl->comment = value;
}

void SvgCommentElement::toXml(SvgRope &output) const
{
    std::string buffer = std::format("<!--{}-->", _impl->comment);
    output.push_back(std::move(buffer));
    //  Comments cannot have children or attributes
}

SvgInterface::SvgType SvgCommentElement::elementType() const
{
    return _impl->elementType;
}

void SvgCommentElement::setElementType(SvgInterface::SvgType elementType)
{
    _impl->elementType = elementType;
}
