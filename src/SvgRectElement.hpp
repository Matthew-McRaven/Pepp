#pragma once

#include <list>
#include <memory>
#include <string>

#include "SvgElement.hpp"
#include "utility_p.hpp"

//	Forward declarations
class SvgRectElementImpl;

class SvgRectElement : public SvgElement
{
    SvgRectElementImpl *derivedThis();
    const SvgRectElementImpl *derivedThis() const;

public:
    SvgRectElement();
    explicit SvgRectElement(const std::string &name, const std::string &value = "");
    ~SvgRectElement();
    //Cannot copy, but can move
    SvgRectElement(const SvgRectElement &);
    SvgRectElement &operator=(const SvgRectElement &);
    SvgRectElement(SvgRectElement &&) noexcept;
    SvgRectElement &operator=(SvgRectElement &&) noexcept;

    //	User access functions
    //  Values can be changed, but not units of measure (yet)
    auto rx() const;
    void setRX(double x = 0);
    void setRX(const std::string_view sv);
    auto ry() const;
    void setRY(double y = 0);
    void setRY(const std::string_view sv);

    void toXml(SvgRope &output) const override;
    bool setAttribute(const std::string &key, const std::string &value) override;
};
