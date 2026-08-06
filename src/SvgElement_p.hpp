#pragma once

//  W A R N I N G
//  -------------
//
// SvgElement uses PIMPL pattern to manage data access. Data and functions
// in this header are not part of the public interface and may
// change without notice.

//	Standard Library

#include <string>
#include <unordered_map>

//	private classes
#include "SvgElement.hpp"

class SvgElementImpl
{
public:
    SvgElementImpl() {}

    ~SvgElementImpl() = default;
    SvgElementImpl(const SvgElementImpl &) = default;
    SvgElementImpl &operator=(const SvgElementImpl &) = default;
    SvgElementImpl(SvgElementImpl &&) noexcept = default;
    SvgElementImpl &operator=(SvgElementImpl &&) noexcept = default;

    std::string id;
    std::string className;
    //SVGElement ownerSVGElement
    std::unordered_map<std::string, std::string> attributes;
};
