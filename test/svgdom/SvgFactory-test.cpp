#include <catch.hpp>

#include <iostream>
#include <typeinfo> //  typeid()

#include "svgdom/SvgCDataElement.hpp"
#include "svgdom/SvgCommentElement.hpp"
#include "svgdom/SvgDocument.hpp"
#include "svgdom/SvgFactory.hpp"
#include "svgdom/SvgRectElement.hpp"
#include "svgdom/SvgSvgElement.hpp"
#include "svgdom/SvgUseElement.hpp"

TEST_CASE("SvgFactor tests-createElementAs", "[scope:core][scope:core.svgdom.svgfactory][kind:unit]") {
  //  Test that SvgFactory class perform correctly

  // std::cout << "From SvgFactory" << std::endl;
  //   Test element creation through SvgElment class. This is interface users will access.
  for (const auto typePair : SvgType::typeMap) {

    //  Create dummy document
    SvgDocument doc{};
    auto &svg = doc.documentElement();

    //  Check that type matches enum
    switch (typePair.first) {
    case SvgType::Type::SvgCommentElement: {
      auto element = svg.createElementAs<SvgCommentElement>(SvgType::Type::SvgCommentElement);
      CHECK(element->elementType() == typePair.first);
      CHECK(typeid(*element).name() == typeid(SvgCommentElement).name());
      break;
    }
    case SvgType::Type::SvgCDataElement: {
      auto element = svg.createElementAs<SvgCDataElement>(SvgType::Type::SvgCDataElement);
      CHECK(element->elementType() == typePair.first);
      CHECK(typeid(*element).name() == typeid(SvgCDataElement).name());
      break;
    }
    case SvgType::Type::SvgRectElement: {
      auto element = svg.createElementAs<SvgRectElement>(SvgType::Type::SvgRectElement);
      CHECK(element->elementType() == typePair.first);
      CHECK(typeid(*element).name() == typeid(SvgRectElement).name());
      break;
    }
    case SvgType::Type::SvgSvgElement: {
      auto element = svg.createElementAs<SvgSvgElement>(SvgType::Type::SvgSvgElement);
      CHECK(element->elementType() == typePair.first);
      CHECK(typeid(*element).name() == typeid(SvgSvgElement).name());
      break;
    }
    case SvgType::Type::SvgUseElement: {
      auto element = svg.createElementAs<SvgUseElement>(SvgType::Type::SvgUseElement);
      CHECK(element->elementType() == typePair.first);
      CHECK(typeid(*element).name() == typeid(SvgUseElement).name());
      break;
    }

    default: {
      auto element = svg.createElementAs<SvgElement>(typePair.first);
      CHECK(element->elementType() == typePair.first);
      CHECK(typeid(*element).name() == typeid(SvgElement).name());
      break;
    }
    }
  }
}

TEST_CASE("SvgFactor tests-createElement", "[scope:core][scope:core.svgdom.svgfactory][kind:unit]") {
  //  Test that SvgFactory class perform correctly

  // std::cout << "From SvgFactory" << std::endl;
  //  Call create element directly SvgFactory
  //  Type is always SvgInterface
  for (const auto typePair : SvgType::typeMap) {

    auto element = SvgFactory::createElement(typePair.first);
    CHECK(element->elementType() == typePair.first);
  }
}