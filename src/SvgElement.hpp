#pragma once

#include <list>
#include <memory>
#include <string>

#include "utility_p.hpp"

//	Forward declarations
class SvgElementImpl;
class XmlAttributes;

class SvgElement
{
protected:
    std::unique_ptr<SvgElementImpl> _impl;
public:
    enum class SvgType {
        SvgUnknownElement = 0,
        //  Dom elements
        DomComment,
        //  SvgSpecific elements
        SvgElementParent = 0x0100,
        SvgDescElement,
        SvgMetadataElement,
        SvgStyleElement,
        SvgTitleElement,
        SvgGraphicElement = 0x8000,
        SvgSvgElement,
        //  Graphic elements
        SvgGeometry = 0x8100,
        SvgGElement,
        SvgDefsElement,
        SvgSymbolElement,
        SvgUseElement,
        SvgSwitchElement,
        SvgCircleElement,
        SvgLineElement,
        SvgPathElement,
        SvgPolygonElement,
        SvgRectElement,
        SvgTextElement,
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
    //  Standard Xml
    SvgElement::SvgType elementType() const;
    void setElementType(SvgElement::SvgType elementType);
    std::string &xmlName() const;
    void setXmlName(std::string xmlName);
    std::string &value() const;
    void setValue(std::string value);

    //  Svg specific functions
    std::string &id() const;
    void setId(std::string id);
    std::string &className() const;
    void setClassName(std::string className);
    std::string &title() const;
    void setTitle(std::string title);
    std::string &metadata() const;
    void setMetadata(std::string metadata);
    std::string &desc() const;
    void setDesc(std::string desc);

    //	User access functions
    //  Values can be changed, but not units of measure (yet)
    auto x() const;
    void setX(double x = 0);
    void setX(const std::string_view sv);
    auto y() const;
    void setY(double y = 0);
    void setY(const std::string_view sv);
    auto width() const;
    void setWidth(double width = 0);
    void setWidth(const std::string_view sv);
    auto height() const;
    void setHeight(double height = 0);
    void setHeight(const std::string_view sv);

    const XmlAttributes &attributes() const;
    XmlAttributes &attributes();

    const std::list<SvgElement *> &children() const;
    std::list<SvgElement *> &children();
    void appendChild(SvgElement *child);

    virtual void toXml(SvgRope &output) const;
    virtual bool setAttribute(const std::string &key, const std::string &value);
};
