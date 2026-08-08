#pragma once

//  W A R N I N G
//  -------------
//
// SvgElement uses PIMPL pattern to manage data access. Data and functions
// in this header are not part of the public interface and may
// change without notice.

//	Standard Library

#include <list>
#include <string>
#include <utility>

using pair = std::pair<std::string, std::string>;

//	private classes
#include "SvgElement.hpp"

class SvgElementImpl
{
public:
    SvgElementImpl() {}

    virtual ~SvgElementImpl() = default;
    SvgElementImpl(const SvgElementImpl &) = default;
    SvgElementImpl &operator=(const SvgElementImpl &) = default;
    SvgElementImpl(SvgElementImpl &&) noexcept = default;
    SvgElementImpl &operator=(SvgElementImpl &&) noexcept = default;

    SvgElement::SvgType elementType = SvgElement::SvgType::SvgUnknownElement;

    std::string id;
    std::string className;
    std::string value;
    std::string type;
    std::string title;
    std::string metadata;
    std::string desc;
    //SVGElement ownerSVGElement
    std::list<pair> attributes;
    std::list<SvgElement *> elements;
};
