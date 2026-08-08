#pragma once

#include <list>
#include <memory>
#include <string>

#include "SvgElement.hpp"

//	Forward declarations
class SvgSvgElementImpl;

class SvgSvgElement : public SvgElement
{
    //std::unique_ptr<SvgSvgElementImpl> _impl;

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

    //virtual void toXml(std::list<std::string> &output) const override;
};
