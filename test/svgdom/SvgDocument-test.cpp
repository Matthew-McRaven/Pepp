#include <catch.hpp>

#include <iostream>
using namespace std::string_literals;

#include "svgdom/SvgDocument.hpp"

TEST_CASE("SvgDocument tests", "[scope:core][scope:core.svgdom.svgdocument][kind:unit]") {
  //  Test that SvgDocument class perform correctly

  // std::cout << "From SvgDocument" << std::endl;
  SvgDocument e;

  /*  e.setXmlName("Name1"s);
    CHECK(e.xmlName() == "Name1"s);

    //  SvgUseElement does not use value
    e.setValue("Value1"s);
    CHECK(e.value() == ""s);

    SvgUseElement e2("Name2"s, "Value2"s);
    CHECK(e2.elementType() == SvgType::Type::SvgUseElement);
    CHECK(e2.xmlName() == "Name2"s);
    CHECK(e2.value() == ""s);

    e2.setHref("#ref1");
    CHECK(e2.href() == "#ref1"s);
    e2.setHref("ref2");
    CHECK(e2.href() == "#ref2"s);

    SvgRope data;
    e2.toXml(data);
    CHECK(data.toString() == "<use href=\"#ref2\" />"s);

    e.setAttribute("K1"s, "V1"s);
    e.setAttribute("K2"s, "V2"s);
    CHECK(e.attributes().size() == 2);

    data.clear();
    e.toXml(data);
    CHECK(data.toString() == "<use K1=\"V1\" K2=\"V2\" />"s);*/
}