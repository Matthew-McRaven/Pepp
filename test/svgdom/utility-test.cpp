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

  {
    //  Test toString/fromString
    auto xml = list.toString();
    SvgList list2;
    CHECK(list2.fromString(xml));
    CHECK(list2.size() == 4);
  }

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

TEST_CASE("Utility file-SvgUnitValue tests", "[scope:core][scope:core.svgdom][kind:unit]") {
  //  Test that utility classes perform correctly
  //  Test SvgUnitValue class

  //  Constructor tests
  {
    SvgUnitValue t(1);
    CHECK(t.toString() == "1"s);
  }
  {
    SvgUnitValue t(-0.5);
    CHECK(t.toString() == "-0.5"s);
  }
  {
    SvgUnitValue t(50, SvgUnits::SvgUnit::in);
    CHECK(t.toString() == "50in"s);
  }

  SvgUnitValue t1;
  CHECK(t1.empty());
  CHECK(t1.toString() == ""s);

  t1.set(1);
  CHECK(t1.toString() == "1"s);

  t1.set(-0.5);
  CHECK(t1.toString() == "-0.5"s);

  t1.set(50, SvgUnits::SvgUnit::pct);
  CHECK(t1.toString() == "50%"s);

  //  Parsing tests
  CHECK(t1.fromString("1"s));
  CHECK(t1.toString() == "1"s);

  CHECK(t1.fromString("-0.5"s));
  CHECK(t1.toString() == "-0.5"s);

  CHECK(t1.fromString("-50mm"));
  CHECK(t1.toString() == "-50mm"s);

  CHECK(!t1.fromString("bad value"));
}

TEST_CASE("Utility file-SvgRect tests", "[scope:core][scope:core.svgdom][kind:unit]") {
  //  Test that utility classes perform correctly
  //  Test SvgRect class

  //  Constructor tests
  {
    SvgRect r1;
    CHECK(r1.empty());
  }

  SvgRect r(1, 2, 3, 4);
  CHECK(!r.empty());
  CHECK(r.x().toString() == "1"s);
  CHECK(r.y().toString() == "2"s);
  CHECK(r.width().toString() == "3"s);
  CHECK(r.height().toString() == "4"s);

  //  Test getter/setter
  r.setX(-100);
  CHECK(r.x().toString() == "-100"s);
  r.setY(20.1);
  CHECK(r.y().toString() == "20.1"s);
  r.setWidth(30.1);
  CHECK(r.width().toString() == "30.1"s);
  r.setHeight(400);
  CHECK(r.height().toString() == "400"s);

  //  Test negative
  r.setWidth(-30.1);
  CHECK(r.width().toString() == "0"s);
  r.setHeight(-400);
  CHECK(r.height().toString() == "0"s);

  //  Test parsing
  r.setX("-100");
  CHECK(r.x().toString() == "-100"s);
  r.setY("20.1");
  CHECK(r.y().toString() == "20.1"s);
  r.setWidth("30.1%");
  CHECK(r.width().toString() == "30.1%"s);
  r.setHeight("400mm");
  CHECK(r.height().toString() == "400mm"s);

  //  String functions
  const std::string input = "0.1 2mm 30% 40in";
  CHECK(r.fromString(input));
  CHECK(r.x().toString() == "0.1"s);
  CHECK(r.y().toString() == "2mm"s);
  CHECK(r.width().toString() == "30%"s);
  CHECK(r.height().toString() == "40in"s);

  CHECK(r.toString() == input);

  //  Too few parameters
  {
    const std::string badInput1 = "0.1 2mm 30%";
    CHECK(!r.fromString(badInput1));
  }
  //  Amounts missing parameter
  {
    const std::string badInput2 = "0.1 mm % in";
    CHECK(!r.fromString(badInput2));
  }
  {
    //  Missing input
    const std::string badInput3 = "";
    CHECK(!r.fromString(""s));
    CHECK(!r.fromString(" "s));
  }
  {
    //  Too many parameters
    const std::string badInput4 = "0.1 2mm 30% 40in 50";
    CHECK(!r.fromString(badInput4));
  }
}

TEST_CASE("Utility file-SvgRope tests", "[scope:core][scope:core.svgdom][kind:unit]") {
  //  Test that utility classes perform correctly
  //  Test SvgRope class

  SvgRope rope;
  CHECK(rope.size() == 0);

  std::string temp = "1234"s;
  rope.push_back(temp);
  rope.push_back("5678"s);
  rope.push_back("90"s);
  CHECK(rope.size() == 10);

  rope.clear();
  CHECK(rope.size() == 0);
}
