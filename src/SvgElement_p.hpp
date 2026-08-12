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

//	private classes
#include "SvgElement.hpp"
#include "XmlAttributes_p.hpp"
#include "utility_p.hpp"

class SvgElementImpl
{
public:
    SvgElementImpl() {}

    virtual ~SvgElementImpl() = default;
    SvgElementImpl(const SvgElementImpl &) = default;
    SvgElementImpl &operator=(const SvgElementImpl &) = default;
    SvgElementImpl(SvgElementImpl &&) noexcept = default;
    SvgElementImpl &operator=(SvgElementImpl &&) noexcept = default;

    //  Standard Xml Data
    SvgElement::SvgType elementType = SvgElement::SvgType::SvgUnknownElement;
    std::string xmlName;
    std::string value;

    //  Svg specific data
    std::string id;
    std::string className;
    std::string title;
    std::string metadata;
    std::string desc;

    SvgUnitValue x;
    SvgUnitValue y;
    SvgUnitValue width;
    SvgUnitValue height;

    //SVGElement ownerSVGElement
    //  Used to store unprocessed xml elements
    std::list<SvgElement *> elements;
    XmlAttributes attributes;
};
