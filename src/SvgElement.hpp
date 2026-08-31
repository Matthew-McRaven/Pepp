#pragma once

#include <list>
#include <string>

#include "XmlAttributes_p.hpp"
#include "utility_p.hpp"

//	Forward declarations
class XmlAttributes;

class SvgElement
{
public:
    enum class SvgType {
        SvgUnknownElement = 0,
        //  Dom elements
        SvgCommentElement,
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

    SvgElement() = default;
    explicit SvgElement(const std::string &name, const std::string &value = "");
    virtual ~SvgElement() = default;
    //Cannot copy, but can move
    SvgElement(const SvgElement &) = default;
    SvgElement &operator=(const SvgElement &) = default;
    SvgElement(SvgElement &&) noexcept = default;
    SvgElement &operator=(SvgElement &&) noexcept = default;

    //	User access functions
    //  Standard Xml
    SvgElement::SvgType elementType() const;
    void setElementType(SvgElement::SvgType elementType);
    const std::string &xmlName() const;
    void setXmlName(const std::string &xmlName);
    const std::string &value() const;
    void setValue(const std::string &value);

    //  Svg specific functions
    const std::string &id() const;
    void setId(const std::string &id);
    const std::string &className() const;
    void setClassName(const std::string &className);
    const std::string &title() const;
    void setTitle(const std::string &title);
    const std::string &metadata() const;
    void setMetadata(const std::string &metadata);
    const std::string &desc() const;
    void setDesc(const std::string &desc);

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

    const auto &children() const;
    auto &children();
    //void appendChild(SvgElement *child);
    SvgElement *createElement(const std::string &name);

    //  Overrides
    virtual void toXml(SvgRope &output) const;
    virtual bool setAttribute(const std::string &key, const std::string &value);
    virtual bool attributeXml(SvgRope &output) const;

protected:
    //  Standard Xml Data
    SvgElement::SvgType _elementType = SvgElement::SvgType::SvgUnknownElement;
    std::string _xmlName;
    std::string _value;

    //  Svg specific data
    std::string _id;
    std::string _className;

    //  Stored as Elements in svg
    SvgElement *_title{};
    SvgElement *_metadata{};
    SvgElement *_desc{};

    SvgUnitValue _x;
    SvgUnitValue _y;
    SvgUnitValue _width;
    SvgUnitValue _height;

    //SVGElement ownerSVGElement
    //  Used to store unprocessed xml elements
    std::list<std::unique_ptr<SvgElement>> _children;
    XmlAttributes _attributes;

    //  Returned by string functions when empy string is needed
    std::string _empty;
};
