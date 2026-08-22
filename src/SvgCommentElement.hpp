#pragma once

#include <list>
#include <memory>
#include <string>

#include "SvgInterface.h"

//	Forward declarations
class SvgCommentElementImpl;

class SvgCommentElement : public SvgInterface
{
    std::unique_ptr<SvgCommentElementImpl> _impl;

public:
    SvgCommentElement();
    explicit SvgCommentElement(const std::string &comment);
    ~SvgCommentElement();
    //Cannot copy, but can move
    SvgCommentElement(const SvgCommentElement &);
    SvgCommentElement &operator=(const SvgCommentElement &);
    SvgCommentElement(SvgCommentElement &&) noexcept;
    SvgCommentElement &operator=(SvgCommentElement &&) noexcept;

    //	User access functions
    //  Standard Xml
    SvgInterface::SvgType elementType() const;
    void setElementType(SvgInterface::SvgType elementType);
    std::string &comment() const;
    void setComment(std::string comment);

    //  Called by base class
    void toXml(SvgRope &output) const override;
    std::unique_ptr<SvgInterface> clone() const;
};
