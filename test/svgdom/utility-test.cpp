#include <catch.hpp>

#include "svgdom/utility_p.hpp"

TEST_CASE("Utility file-SvgList tests", "[scope:core][scope:core.svgdom][kind:unit]") {
  //  Test that utility classes perform correctly
  //  Test SvgList class

  SvgList list;

  REQUIRE(list.empty());
  list.add("first"s);
  CHECK(!list.empty());
  CHECK(list.size() == 1);

  list.add("second"s);
  list.add("third"s);
  list.add("fourth"s);

  //  Test duplicates
  list.add("first"s);
  CHECK(list.size() == 4);

  list.remove("third"s);
  CHECK(list.size() == 3);

  CHECK(!list.contains("third"s));
  CHECK(list.contains("second"s));

  list.toggle("second"s);
  CHECK(list.size() == 2);

  list.toggle("last"s);
  CHECK(list.size() == 3);

  list.remove("fourth"s);
  CHECK(list.size() == 2);

  CHECK(list.toString() == "first last");
  list.remove("first"s);

  CHECK(list.toString() == "last");
}

TEST_CASE("Utility file-SvgUnits tests", "[scope:core][scope:core.svgdom][kind:unit]") {
  //  Test that utility classes perform correctly
  //  Test SvgUnits class

  constexpr std::array<SvgUnits::SvgUnit, 9> allUnits = {
      SvgUnits::SvgUnit::None, SvgUnits::SvgUnit::cm,  SvgUnits::SvgUnit::mm,
      SvgUnits::SvgUnit::Q,    SvgUnits::SvgUnit::in,  SvgUnits::SvgUnit::pc,
      SvgUnits::SvgUnit::pt,   SvgUnits::SvgUnit::pct, SvgUnits::SvgUnit::px};

  //  Check all existing enums
  for (const auto unit : allUnits) {
    CHECK(unit == SvgUnits::fromString(SvgUnits::toString(unit)));
  }

  CHECK(SvgUnits::fromString("invalid value") == SvgUnits::SvgUnit::None);
}
