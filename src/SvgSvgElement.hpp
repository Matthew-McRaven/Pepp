#pragma once

#include <list>
#include <string>

#include "SvgElement.hpp"
#include "utility_p.hpp"

class SvgSvgElement final : public Cloneable<SvgSvgElement, SvgElement>
{
    SvgRect _viewBox;
    std::string _xmlns = "http://www.w3.org/2000/svg";

public:
    SvgSvgElement(); // = default;
    explicit SvgSvgElement(const std::string &name, const std::string &value = "");
    ~SvgSvgElement() = default;
    //  Can copy and move
    SvgSvgElement(const SvgSvgElement &) = default;
    SvgSvgElement &operator=(const SvgSvgElement &) = default;
    SvgSvgElement(SvgSvgElement &&) noexcept = default;
    SvgSvgElement &operator=(SvgSvgElement &&) noexcept = default;

    //	User access functions
    SvgRect &viewBox();
    const SvgRect &viewBox() const;

    void toXml(SvgRope &output) const override;
    bool setAttribute(const std::string &key, const std::string &value) override;
    bool attributeXml(SvgRope &output) const override;
};
