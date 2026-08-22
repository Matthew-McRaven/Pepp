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
#include "SvgCommentElement.hpp"

class SvgCommentElementImpl
{
public:
    SvgCommentElementImpl() {}

    ~SvgCommentElementImpl() = default;
    SvgCommentElementImpl(const SvgCommentElementImpl &) = default;
    SvgCommentElementImpl &operator=(const SvgCommentElementImpl &) = default;
    SvgCommentElementImpl(SvgCommentElementImpl &&) noexcept = default;
    SvgCommentElementImpl &operator=(SvgCommentElementImpl &&) noexcept = default;

    //  Standard Xml Data
    SvgInterface::SvgType elementType = SvgInterface::SvgType::SvgCommentElement;
    std::string comment;
};
