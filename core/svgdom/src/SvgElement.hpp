#pragma once

#include <list>
#include <string>

#include "SvgInterface.hpp"
#include "XmlAttributes_p.hpp"
#include "utility_p.hpp"

//	Forward declarations
class XmlAttributes;
class Document;

class SvgElement : public Cloneable<SvgElement, SvgInterface>
{
public:
    SvgElement() = default;
    explicit SvgElement(const std::string &name, const std::string &value = "");
    virtual ~SvgElement() = default;
    //Can copy and move
    SvgElement(const SvgElement &); // = default;
    SvgElement &operator=(const SvgElement &) = default;
    SvgElement(SvgElement &&) noexcept = default;
    SvgElement &operator=(SvgElement &&) noexcept = default;

    //	User access functions
    //  Standard Xml
    SvgType::Type elementType() const override;
    void setElementType(const SvgType::Type elementType) override;
    const std::string &xmlName() const override;
    void setXmlName(const std::string &xmlName) override;
    const std::string &value() const override;
    void setValue(const std::string &value) override;
    void setDocument(Document *doc) override;

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
    SvgUnitValue x() const;
    void setX(double x = 0);
    void setX(const std::string_view sv);
    SvgUnitValue y() const;
    void setY(double y = 0);
    void setY(const std::string_view sv);
    SvgUnitValue width() const;
    void setWidth(double width = 0);
    void setWidth(const std::string_view sv);
    SvgUnitValue height() const;
    void setHeight(double height = 0);
    void setHeight(const std::string_view sv);

    const XmlAttributes &attributes() const;
    XmlAttributes &attributes();

    const std::list<std::unique_ptr<SvgInterface>> &children() const;
    std::list<std::unique_ptr<SvgInterface>> &children();
    SvgInterface *createElement(const std::string &name) override;
    SvgInterface *createElement(const SvgType::Type type) override;
    void appendChild(SvgInterface *child) override;

    //  Overrides
    virtual void toXml(SvgRope &output) const override;
    virtual bool setAttribute(const std::string &key, const std::string &value) override;
    virtual bool attributeXml(SvgRope &output) const;

private:
    //  Standard Xml Data
    SvgType::Type _elementType = SvgType::Type::SvgUnknownElement;
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

    Document *_doc{};

    //SVGElement ownerSVGElement

    //  Used to store unprocessed xml elements
    std::list<std::unique_ptr<SvgInterface>> _children;
    XmlAttributes _attributes;

    //  Returned by string functions when empy string is needed
    std::string _empty;
};
