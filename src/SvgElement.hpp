#pragma once

#include <list>
#include <memory>
#include <string>

//	Forward declarations
class SvgElementImpl;

class SvgElement
{
    std::unique_ptr<SvgElementImpl> _impl;

public:
    enum class SvgType {
        SvgUnknownElement = 0,
        SvgElementParent = 1,
        SvgDescElement,
        SvgMetadataElement,
        SvgStyleElement,
        SvgTitleElement,
        SvgGraphicElement = 0x8000,
        SvgSvgElement,
        SvgGeometry = 0x8100,
        SvgGElement,
        SvgDefsElement,
        SvgSymbolElement,
        SvgUseElement,
        SvgSwitchElement
    };

    SvgElement();
    explicit SvgElement(const std::string &name, const std::string &value = "");
    ~SvgElement();
    //Cannot copy, but can move
    SvgElement(const SvgElement &);
    SvgElement &operator=(const SvgElement &);
    SvgElement(SvgElement &&) noexcept;
    SvgElement &operator=(SvgElement &&) noexcept;

    //	User access functions
    std::string id() const;
    void setId(std::string id);
    std::string className() const;
    void setClassName(std::string className);
    std::string value() const;
    void setValue(std::string value);

    auto attributes() const;
    auto attributes();

    auto children() const;
    auto children();
    void appendChild(SvgElement *child);

    void toXml(std::list<std::string> &output) const;
};
