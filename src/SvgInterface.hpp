#pragma once

#include <memory>
#include <string>
using namespace std::string_literals;

class SvgRope;

// CRTP helper class that implements clone() automatically
template<class Derived, class Base>
struct Cloneable : public Base
{
    //friend class SvgInterface;
    Base *cloneImpl() const override { return new Derived(static_cast<Derived const &>(*this)); }
};

class Document;

class SvgInterface
{
    //  Base class used to enforce enterface used by derived Svg elements
protected:
    virtual SvgInterface *cloneImpl() const = 0;

public:
    enum class SvgType {
        SvgUnknownElement = 0,
        //  Dom elements
        SvgBasicElement,
        SvgCommentElement,
        SvgCDataElement,
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

    virtual ~SvgInterface() = default;

    //  Overrides
    virtual void toXml(SvgRope &output) const = 0;

    //  Values in every class
    virtual void setValue(const std::string &key) = 0;
    virtual void setDocument(Document *doc) = 0;
    virtual bool setAttribute(const std::string &key, const std::string &value) = 0;
    virtual SvgInterface::SvgType elementType() const = 0;
    virtual void setElementType(SvgInterface::SvgType elementType) = 0;

    template<typename Self>
    std::unique_ptr<Self> clone(this const Self &self)
    {
        return std::unique_ptr<Self>(static_cast<Self *>(self.cloneImpl()));
    }

    virtual SvgInterface *createElement(const std::string &name) = 0;
};