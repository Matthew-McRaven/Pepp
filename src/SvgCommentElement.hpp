#pragma once

#include <string>

#include "SvgInterface.h"

//	Forward declarations
class SvgCommentElementImpl;
class DocumentImpl;

class SvgCommentElement final : public SvgInterface
{
    //  Pointer to document for callbacks
    DocumentImpl *_doc{};

    //  Standard Xml Data
    SvgInterface::SvgType _elementType = SvgInterface::SvgType::SvgCommentElement;
    std::string _comment;

public:
    SvgCommentElement();
    explicit SvgCommentElement(DocumentImpl *d);
    explicit SvgCommentElement(const std::string &comment);
    ~SvgCommentElement() = default;
    //Cannot copy, but can move
    SvgCommentElement(const SvgCommentElement &) = default;
    SvgCommentElement &operator=(const SvgCommentElement &) = default;
    SvgCommentElement(SvgCommentElement &&) noexcept = default;
    SvgCommentElement &operator=(SvgCommentElement &&) noexcept = default;

    //	User access functions
    //  Standard Xml
    const std::string &comment() const;
    void setComment(std::string comment);

    //  Called by base class
    void toXml(SvgRope &output) const override;
    bool setAttribute(const std::string &key, const std::string &value) override { return true; };
    SvgInterface::SvgType elementType() const override;
    void setElementType(SvgInterface::SvgType elementType) override;

    //std::unique_ptr<SvgInterface> clone() const;
};
