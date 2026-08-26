#pragma once

#include <memory>
#include <string>

#include "SvgInterface.h"

//	Forward declarations
class SvgCommentElementImpl;
class DocumentImpl;

class SvgCommentElement : public SvgInterface
{
    std::unique_ptr<SvgCommentElementImpl> _impl;

public:
    SvgCommentElement();
    explicit SvgCommentElement(DocumentImpl *d);
    explicit SvgCommentElement(const std::string &comment);
    ~SvgCommentElement();
    //Cannot copy, but can move
    SvgCommentElement(const SvgCommentElement &);
    SvgCommentElement &operator=(const SvgCommentElement &);
    SvgCommentElement(SvgCommentElement &&) noexcept;
    SvgCommentElement &operator=(SvgCommentElement &&) noexcept;

    //	User access functions
    //  Standard Xml
    std::string &comment() const;
    void setComment(std::string comment);

    //  Called by base class
    void toXml(SvgRope &output) const override;
    bool setAttribute(const std::string &key, const std::string &value) override { return true; };
    SvgInterface::SvgType elementType() const override;
    void setElementType(SvgInterface::SvgType elementType) override;

    std::unique_ptr<SvgInterface> clone() const;
};
