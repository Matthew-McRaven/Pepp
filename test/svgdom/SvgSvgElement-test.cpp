#include <catch.hpp>

#include <iostream>
using namespace std::string_literals;

#include "svgdom/SvgSvgElement.hpp"

TEST_CASE("SvgSvgElement tests", "[scope:core][scope:core.svgdom.svgsvgelement][kind:unit]") {
  //  Test that SvgSvgElement class perform correctly

  // std::cout << "From SvgSvgElement" << std::endl;
  SvgSvgElement e;
  CHECK(e.elementType() == SvgType::Type::SvgSvgElement);

  e.setXmlName("Name1"s);
  CHECK(e.xmlName() == "Name1"s);
  //  Value is not valid for SvgSvgElement
  e.setValue("Value1"s);
  CHECK(e.value() == ""s);

  SvgSvgElement e2("Name2"s, "Value2"s);
  CHECK(e2.elementType() == SvgType::Type::SvgSvgElement);
  CHECK(e2.xmlName() == "Name2"s);
  CHECK(e2.value() == ""s);
  e2.viewBox().fromString("1 2 3 4");

  SvgRope data;
  e2.toXml(data);
  CHECK(data.toString() == "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"1 2 3 4\" />"s);

  //  CData can have not attributes or children
  e.setAttribute("K1"s, "V1"s);
  e.setAttribute("K2"s, "V2"s);
  CHECK(e.attributes().size() == 2);

  data.clear();
  e.toXml(data);
  CHECK(data.toString() ==
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100% 100%\" K1=\"V1\" K2=\"V2\" />");
}