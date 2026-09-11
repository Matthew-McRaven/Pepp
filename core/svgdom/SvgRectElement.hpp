#pragma once

#include <string>

#include "SvgElement.hpp"
#include "utility_p.hpp"

class SvgRectElement final : public Cloneable<SvgRectElement, SvgElement>
{
    SvgUnitValue _rx;
    SvgUnitValue _ry;

public:
    SvgRectElement();
    explicit SvgRectElement(const std::string &name, const std::string &value = "");
    ~SvgRectElement() = default;
    //Allo copy and move
    SvgRectElement(const SvgRectElement &) = default;
    SvgRectElement &operator=(const SvgRectElement &) = default;
    SvgRectElement(SvgRectElement &&) noexcept = default;
    SvgRectElement &operator=(SvgRectElement &&) noexcept = default;

    //	User access functions
    //  Values can be changed, but not units of measure (yet)
    auto rx() const;
    void setRx(double x = 0);
    void setRx(const std::string_view sv);
    auto ry() const;
    void setRy(double y = 0);
    void setRy(const std::string_view sv);

    void toXml(SvgRope &output) const override;
    bool setAttribute(const std::string &key, const std::string &value) override;
    bool attributeXml(SvgRope &output) const override;
};
