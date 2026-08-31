#pragma once

#include <string>

#include "SvgElement.hpp"

class SvgCommentElement final : public SvgElement
{
public:
    SvgCommentElement();
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
};
