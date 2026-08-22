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
#include "SvgBasicElement.hpp"
//#include "utility_p.hpp"

class SvgBasicElementImpl
{
public:
    SvgBasicElementImpl() {}

    /*virtual*/ ~SvgBasicElementImpl() = default;
    SvgBasicElementImpl(const SvgBasicElementImpl &) = default;
    SvgBasicElementImpl &operator=(const SvgBasicElementImpl &) = default;
    SvgBasicElementImpl(SvgBasicElementImpl &&) noexcept = default;
    SvgBasicElementImpl &operator=(SvgBasicElementImpl &&) noexcept = default;

    //  Standard Xml Data
    SvgInterface::SvgType elementType = SvgInterface::SvgType::SvgUnknownElement;
    std::string xmlName;
    std::string value;
};
