#include <catch.hpp>

#include <iostream>
using namespace std::string_literals;

#include "svgdom/SvgColor.hpp"

TEST_CASE("SvgColor tests", "[scope:core][scope:core.svgdom][kind:unit]") {
  //  Test that Color class perform correctly

  std::cout << "From Svg" << std::endl;
  //  Check all existing enums
  for (const auto colorPair : SvgColor::colorMap) {
    CHECK(colorPair.first == SvgColor::fromName(colorPair.second));
    if (colorPair.first == SvgColor::Color::Aqua) {
      //  Aqua and Cyan have the same color value
      //  Cyan is retunred for Aqua
      CHECK("Aqua" == SvgColor::fromRgb(colorPair.first));
    } else if (colorPair.first == SvgColor::Color::Magenta) {
      //  Fuchsia and Magenta have the same color value
      //  Fuchsia is returned for Magenta
      CHECK("Fuchsia" == SvgColor::fromRgb(colorPair.first));
    } else CHECK(colorPair.second == SvgColor::fromRgb(colorPair.first));
  }

  //  Test unnamed value
  uint32_t somecolor = 0x123456;
  CHECK(SvgColor::formatRgb(somecolor) == "#123456"s);

  //  Test invalid values
  CHECK(SvgColor::fromRgb(somecolor) == "#123456"s);
  CHECK(SvgColor::fromRgb(SvgColor::Color(somecolor)) == ""s);
  CHECK(SvgColor::fromName("Invalid Color") == SvgColor::Color::Black);
}