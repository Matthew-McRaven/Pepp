#pragma once

#include <memory>

//#include "utility_p.hpp"
class SvgRope;

struct SvgInterface
{
    //  Base class that implements Empty Base Class Optimization
    //  Must have no data elements, or v-table will be created.
    //  See https://en.cppreference.com/cpp/language/crtp for basic
    //  explanation.
    //protected:
    //  Must be inherited
    SvgInterface() = default;

public:
    enum class SvgType {
        SvgUnknownElement = 0,
        //  Dom elements
        SvgBasicElement,
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

    virtual ~SvgInterface() = default;
    void serialize(this auto &&self, SvgRope &output) { self.toXml(output); }
    SvgType type(this auto &&self) { return self.elementType(); }

    //std::unique_ptr<SvgInterface> clone() const;

    //  Call to base class should just return
    SvgType elementType() { return SvgType::SvgUnknownElement; }
    virtual void toXml(SvgRope &output) const = 0; //{ return; }
};