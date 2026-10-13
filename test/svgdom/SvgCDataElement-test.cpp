#include <catch.hpp>

#include <iostream>
using namespace std::string_literals;

#include "svgdom/SvgCDataElement.hpp"

TEST_CASE("SvgCDataElement tests", "[scope:core][scope:core.svgdom.svgcdataelement][kind:unit]") {
  //  Test that SvgCDataElement class perform correctly

  // std::cout << "From SvgCDataElement" << std::endl;
  SvgCDataElement e;
  CHECK(e.elementType() == SvgType::Type::SvgCDataElement);

  e.setValue("CData1"s);
  CHECK(e.value() == "CData1"s);

  SvgCDataElement e2("WithName"s);
  CHECK(e2.elementType() == SvgType::Type::SvgCDataElement);
  CHECK(e2.value() == "WithName"s);

  SvgRope data;
  e2.toXml(data);
  CHECK(data.toString() == "<![CDATA[WithName]]>");

  //  CData can have not attributes or children
  e.setAttribute("Bad"s, "Data"s);
  CHECK(e.attributes().size() == 0);

  data.clear();
  e.toXml(data);
  CHECK(data.toString() == "<![CDATA[CData1]]>");
}