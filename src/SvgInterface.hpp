#pragma once

#include <memory>
#include <string>
using namespace std::string_literals;

class SvgRope;

// CRTP helper class that implements clone() automatically
template<typename Derived, typename Base>
class Cloneable : public Base
{
public:
    std::unique_ptr<Base> clone() const
    {
        // Safe downcast to invoke the correct copy constructor
        return std::make_unique<Derived>(static_cast<const Derived &>(this->cloneImpl()));
    }

protected:
    Base *cloneImpl() const override
    {
        //const auto& self = return static_cast<const Derived&>(*this);
        return new Derived(static_cast<const Derived &>(*this));
    }
};

class Document;

class SvgInterface
{
    //  Base class used to enforce enterface used by derived Svg elements
protected:
    //  Must be inherited
    //SvgInterface() = default;
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

    /*template<typename Self>
    std::unique_ptr<SvgInterface> clone(this const Self &self) // const
    {
        return std::unique_ptr<Self>(cloneImpl());
    }*/
    std::unique_ptr<SvgInterface> clone() const
    {
        return std::unique_ptr<SvgInterface>(cloneImpl());
    }

    virtual SvgInterface *createElement(const std::string &name) = 0;
};