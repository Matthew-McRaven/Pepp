#pragma once

class SvgType
{
public:
  //  Commented out unused types
  enum class Type {
    SvgUnknownElement = 0,
    //  Dom elements
    // SvgBasicElement,
    SvgCommentElement,
    SvgCDataElement,
    //  SvgSpecific elements
    // SvgElementParent = 0x0100,
    SvgDescElement,
    SvgMetadataElement,
    SvgStyleElement,
    SvgTitleElement,
    // SvgGraphicElement = 0x8000,
    SvgSvgElement,
    //  Graphic elements
    // SvgGeometry = 0x8100,
    SvgGElement,
    SvgDefsElement,
    // SvgSymbolElement,
    SvgUseElement,
    // SvgSwitchElement,
    // SvgCircleElement,
    // SvgLineElement,
    // SvgPathElement,
    // SvgPolygonElement,
    SvgRectElement,
    // SvgTextElement,
  };

  static constexpr std::array<std::pair<Type, std::string_view>, 10> typeMap{{{Type::SvgCommentElement, "comment"},
                                                                              {Type::SvgCDataElement, "cdata"},
                                                                              {Type::SvgDescElement, "desc"},
                                                                              {Type::SvgMetadataElement, "metadata"},
                                                                              {Type::SvgTitleElement, "title"},
                                                                              {Type::SvgSvgElement, "svg"},
                                                                              {Type::SvgGElement, "g"},
                                                                              {Type::SvgDefsElement, "defs"},
                                                                              {Type::SvgRectElement, "rect"},
                                                                              {Type::SvgUseElement, "use"}}};

  static std::string fromType(Type key) {
    auto it = std::find_if(typeMap.begin(), typeMap.end(), [key](const auto &pair) { return pair.first == key; });

    if (it != typeMap.end()) {
      return std::string(it->second);
    }

    //  If we got here, user casted integer into invalid color
    return "";
  }

  constexpr static Type fromName(std::string_view key) {
    auto it = std::find_if(typeMap.begin(), typeMap.end(), [key](const auto &pair) { return pair.second == key; });

    if (it != typeMap.end()) {
      return it->first;
    }

    //  Return black
    return Type::SvgUnknownElement;
  }
};
