#include <catch.hpp>

#include <iostream>
using namespace std::string_literals;

#include "svgdom/SvgCommentElement.hpp"

TEST_CASE("SvgCommentElement tests", "[scope:core][scope:core.svgdom.svgcommentelement][kind:unit]") {
  //  Test that SvgCommentElement class perform correctly

  // std::cout << "From SvgCommentElement" << std::endl;
  SvgCommentElement e;
  CHECK(e.elementType() == SvgType::Type::SvgCommentElement);

  e.setValue("My comment"s);
  CHECK(e.value() == "My comment"s);

  SvgCommentElement e2("With a comment"s);
  CHECK(e2.elementType() == SvgType::Type::SvgCommentElement);
  CHECK(e2.value() == "With a comment"s);

  SvgRope data;
  e2.toXml(data);
  CHECK(data.toString() == "<!--With a comment-->");

  //  CData can have not attributes or children
  e.setAttribute("Bad"s, "Data"s);
  CHECK(e.attributes().size() == 0);

  data.clear();
  e.toXml(data);
  CHECK(data.toString() == "<!--My comment-->");
}