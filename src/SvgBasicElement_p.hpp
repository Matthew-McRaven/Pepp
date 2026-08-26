#pragma once

//  W A R N I N G
//  -------------
//
// SvgElement uses PIMPL pattern to manage data access. Data and functions
// in this header are not part of the public interface and may
// change without notice.

//	Standard Library

#include <string>

//	private classes
#include "SvgBasicElement.hpp"

class DocumentImpl;

class SvgBasicElementImpl
{
public:
    SvgBasicElementImpl() {}

    ~SvgBasicElementImpl() = default;
    SvgBasicElementImpl(const SvgBasicElementImpl &) = default;
    SvgBasicElementImpl &operator=(const SvgBasicElementImpl &) = default;
    SvgBasicElementImpl(SvgBasicElementImpl &&) noexcept = default;
    SvgBasicElementImpl &operator=(SvgBasicElementImpl &&) noexcept = default;

    //  Pointer to document for callbacks
    DocumentImpl *doc{};

    //  Standard Xml Data
    SvgInterface::SvgType elementType = SvgInterface::SvgType::SvgUnknownElement;
    std::string xmlName;
    std::string value;
};
