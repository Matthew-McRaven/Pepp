#pragma once

#include <list>
#include <memory>
#include <string>

#include "SvgElement.hpp"
#include "utility_p.hpp"

//	Forward declarations
class SvgSvgElementImpl;

class SvgSvgElement : public SvgElement
{
    SvgSvgElementImpl *derivedThis();
    const SvgSvgElementImpl *derivedThis() const;

public:
    SvgSvgElement();
    explicit SvgSvgElement(const std::string &name, const std::string &value = "");
    ~SvgSvgElement();
    //Cannot copy, but can move
    SvgSvgElement(const SvgSvgElement &);
    SvgSvgElement &operator=(const SvgSvgElement &);
    SvgSvgElement(SvgSvgElement &&) noexcept;
    SvgSvgElement &operator=(SvgSvgElement &&) noexcept;

    //	User access functions
    SvgRect &viewBox();
    const SvgRect &viewBox() const;

    void toXml(std::list<std::string> &output) const override;
    bool setAttribute(const std::string &key, const std::string &value) override;
};
