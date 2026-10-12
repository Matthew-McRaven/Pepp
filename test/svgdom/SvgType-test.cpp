#include <catch.hpp>

#include <iostream>
using namespace std::string_literals;

#include "svgdom/SvgType.hpp"

TEST_CASE("SvgType tests", "[scope:core][scope:core.svgdom.svgtype][kind:unit]") {
  //  Test that Types class perform correctly

  // std::cout << "From SvgType" << std::endl;
  //    Check all existing enums
  for (const auto typePair : SvgType::typeMap) {
    CHECK(typePair.first == SvgType::fromName(typePair.second));
    CHECK(typePair.second == SvgType::fromType(typePair.first));
  }

  //  Test invalid values
  CHECK(SvgType::fromName("Invalid Type"s) == SvgType::Type::SvgUnknownElement);
}