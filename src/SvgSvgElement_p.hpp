#pragma once

//  W A R N I N G
//  -------------
//
// SvgSvgElement uses PIMPL pattern to manage data access. Data and functions
// in this header are not part of the public interface and may
// change without notice.

//	Standard Library
//#include <string>

//	private classes
#include "SvgElement_p.hpp"
#include "SvgSvgElement.hpp"
#include "utility_p.hpp"

class SvgSvgElementImpl : public SvgElementImpl
{
public:
    SvgSvgElementImpl() {}

    virtual ~SvgSvgElementImpl() override = default;
    SvgSvgElementImpl(const SvgSvgElementImpl &) = default;
    SvgSvgElementImpl &operator=(const SvgSvgElementImpl &) = default;
    SvgSvgElementImpl(SvgSvgElementImpl &&) noexcept = default;
    SvgSvgElementImpl &operator=(SvgSvgElementImpl &&) noexcept = default;

    SvgRect viewBox;
};
