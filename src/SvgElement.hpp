#pragma once

#include <cassert>
#include <memory>
#include <string>

//	Forward declarations
class SvgElementImpl;

class SvgElement
{
    std::unique_ptr<SvgElementImpl> _impl;

public:
    SvgElement();
    explicit SvgElement(const std::string &name);
    ~SvgElement();
    //Cannot copy, but can move
    SvgElement(const SvgElement &) = delete;
    SvgElement &operator=(const SvgElement &) = delete;
    SvgElement(SvgElement &&) noexcept;
    SvgElement &operator=(SvgElement &&) noexcept;

    //	User access functions
    auto attributes() const;
    auto attributes();
};
