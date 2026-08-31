#pragma once

#include <string>

#include "SvgElement.hpp"

class SvgCDataElement final : public SvgElement
{
public:
    SvgCDataElement();
    explicit SvgCDataElement(const std::string &value);
    ~SvgCDataElement() = default;
    //Cannot copy, but can move
    SvgCDataElement(const SvgCDataElement &) = default;
    SvgCDataElement &operator=(const SvgCDataElement &) = default;
    SvgCDataElement(SvgCDataElement &&) noexcept = default;
    SvgCDataElement &operator=(SvgCDataElement &&) noexcept = default;

    //  Called by base class
    void toXml(SvgRope &output) const override;
    bool setAttribute(const std::string &key, const std::string &value) override { return true; };
};
