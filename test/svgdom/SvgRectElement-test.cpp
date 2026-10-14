#include <catch.hpp>

#include <iostream>
using namespace std::string_literals;

#include "svgdom/SvgRectElement.hpp"

TEST_CASE("SvgRectElement tests", "[scope:core][scope:core.svgdom.svgrectelement][kind:unit]") {
  //  Test that SvgRectElement class perform correctly

  // std::cout << "From SvgCDataElement" << std::endl;
  SvgRectElement e;
  CHECK(e.elementType() == SvgType::Type::SvgRectElement);

  e.setXmlName("Name1"s);
  CHECK(e.xmlName() == "Name1"s);
  CHECK(e.ry().toString() == ""s);
  CHECK(e.rx().toString() == ""s);
  CHECK(e.x().toString() == ""s);
  CHECK(e.y().toString() == ""s);
  CHECK(e.width().toString() == ""s);
  CHECK(e.height().toString() == ""s);

  SvgRectElement e2("Name2"s);
  CHECK(e2.elementType() == SvgType::Type::SvgRectElement);
  CHECK(e2.xmlName() == "Name2"s);

  //  Getter/setters
  e.setX(1);
  CHECK(e.x().toString() == SvgUnitValue(1).toString());
  e.setX("10%"s);
  CHECK(e.x().toString() == "10%"s);
  e.setY(2);
  CHECK(e.y().toString() == SvgUnitValue(2).toString());
  e.setY("20mm"s);
  CHECK(e.y().toString() == "20mm"s);
  e.setWidth(3);
  CHECK(e.width().toString() == SvgUnitValue(3).toString());
  e.setWidth(-3);
  CHECK(e.width().toString() == SvgUnitValue(0).toString());
  e.setWidth("30in"s);
  CHECK(e.width().toString() == "30in"s);
  e.setHeight(4);
  CHECK(e.height().toString() == SvgUnitValue(4).toString());
  e.setHeight(-4);
  CHECK(e.height().toString() == SvgUnitValue(0).toString());
  e.setHeight("40px"s);
  CHECK(e.height().toString() == "40px"s);
  e.setRx(5);
  CHECK(e.rx().toString() == SvgUnitValue(5).toString());
  e.setRx("50%"s);
  CHECK(e.rx().toString() == "50%"s);

  e.setRy(6);
  CHECK(e.ry().toString() == SvgUnitValue(6).toString());
  e.setRy("60%"s);
  CHECK(e.ry().toString() == "60%"s);

  e.attributes().add("n1"s, "v1"s);
  CHECK(e.attributes().write() == " n1=\"v1\""s);

  //  Update e2
  e2.setRx(50);
  e2.setRy(60);

  SvgRope data;
  e2.toXml(data);
  CHECK(data.toString() == "<rect rx=\"50\" ry=\"60\" />"s);
}