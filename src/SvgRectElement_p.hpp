#pragma once

//  W A R N I N G
//  -------------
//
// SvgRectElement uses PIMPL pattern to manage data access. Data and functions
// in this header are not part of the public interface and may
// change without notice.

//	Standard Library

//	private classes
#include "SvgElement_p.hpp"

class SvgRectElementImpl : public SvgElementImpl
{
public:
    SvgRectElementImpl() {}

    virtual ~SvgRectElementImpl() override = default;
    SvgRectElementImpl(const SvgRectElementImpl &) = default;
    SvgRectElementImpl &operator=(const SvgRectElementImpl &) = default;
    SvgRectElementImpl(SvgRectElementImpl &&) noexcept = default;
    SvgRectElementImpl &operator=(SvgRectElementImpl &&) noexcept = default;

    bool setAttribute(const std::string &key, const std::string &value) override;
    bool attributeXml(SvgRope &output) const override;

    SvgUnitValue rx;
    SvgUnitValue ry;
    SvgRectElementImpl *derivedThis{};
};
