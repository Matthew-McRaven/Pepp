#include "SvgElement.hpp"
#include "SvgElement_p.hpp"

// SvgElement uses PIMPL pattern to manage data access. Data and functions
// in Impl struct are not part of the public interface and may
// change without notice.

//	Standard library
#include <memory>
#include <string>

//	private classes
#include "Timer.h"
/*
From w3.org: https://www.w3.org/TR/SVG2/types.html#InterfaceSVGElement
    
All of the SVG DOM interfaces that correspond directly to elements in the
SVG language (such as the SVGPathElement interface for the ‘path’ element)
derive from the SVGElement interface.
*/

//	Public interface
SvgElement::SvgElement()
    : _impl(std::make_unique<SvgElementImpl>())
{}

SvgElement::SvgElement(const std::string &id)
    : SvgElement()
{
    _impl->id = id;
}

//	Need to move implementation after Impl structure so unique_ptr will see full
//	definition. Otherwise, compiler error
SvgElement::~SvgElement() = default;
SvgElement::SvgElement(SvgElement &&) noexcept = default;
SvgElement &SvgElement::operator=(SvgElement &&) noexcept = default;

auto SvgElement::attributes() const
{
    return _impl->attributes;
}
auto SvgElement::attributes()
{
    return _impl->attributes;
}
