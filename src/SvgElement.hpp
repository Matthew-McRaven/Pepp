#pragma once

#include <list>
#include <memory>
#include <string>

//	Forward declarations
class SvgElementImpl;

class SvgElement
{
protected:
    std::unique_ptr<SvgElementImpl> _impl;
    //SvgElement(SvgElementImpl *derivedImpl);

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

    SvgElement(bool base = true);
    explicit SvgElement(const std::string &name, const std::string &value = "");
    virtual ~SvgElement();
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
    std::string title() const;
    void setTitle(std::string title);
    std::string metadata() const;
    void setMetadata(std::string metadata);
    std::string desc() const;
    void setDesc(std::string desc);

    auto attributes() const;
    auto attributes();

    auto children() const;
    auto children();
    void appendChild(SvgElement *child);

    virtual void toXml(std::list<std::string> &output) const;
};
