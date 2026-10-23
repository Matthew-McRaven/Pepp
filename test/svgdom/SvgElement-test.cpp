#include <catch.hpp>

#include <iostream>
using namespace std::string_literals;

#include "svgdom/SvgDocument.hpp"
#include "svgdom/SvgElement.hpp"

TEST_CASE("SvgElement tests-getter-setter", "[scope:core][scope:core.svgdom.svgelement][kind:unit]") {
  //  Test that SvgElement class perform correctly

  // std::cout << "From SvgElement" << std::endl;
  //  Constructor tests
  SvgElement element;
  CHECK(element.elementType() == SvgType::Type::SvgUnknownElement);
  CHECK(element.xmlName() == ""s);
  CHECK(element.value() == ""s);
  CHECK(element.title() == ""s);
  CHECK(element.desc() == ""s);
  CHECK(element.metadata() == ""s);

  SvgDocument doc("Name"s);
  CHECK(doc.fileName() == "Name"s);
  element.setDocument(&doc);
  CHECK(element.document()->fileName() == doc.fileName());

  SvgElement element2("Name2"s, "Value2"s);
  CHECK(element2.xmlName() == "Name2"s);
  CHECK(element2.value() == "Value2"s);

  //  Getter/setter tests
  element.setXmlName("XmlName"s);
  CHECK(element.xmlName() == "XmlName"s);
  element.setValue("Value"s);
  CHECK(element.value() == "Value"s);
  element.setId("Id"s);
  CHECK(element.id() == "Id"s);
  element.setTitle("Title"s);
  CHECK(element.title() == "Title"s);
  element.setDesc("Desc"s);
  CHECK(element.desc() == "Desc"s);
  element.setMetadata("Metadata"s);
  CHECK(element.metadata() == "Metadata"s);

  CHECK(element.fill() == SvgColor::Color::Unknown);
  element.setFill(SvgColor::Color::AntiqueWhite);
  CHECK(element.fill() == SvgColor::Color::AntiqueWhite);
  CHECK(element.opacity() == 1.0);
  element.setOpacity(0.4);
  CHECK(element.opacity() == 0.4);
  element.setOpacity(50);
  CHECK(element.opacity() == 1.0);
  element.setOpacity(-5.5);
  CHECK(element.opacity() == 0.0);

  element.setX(1);
  CHECK(element.x().toString() == SvgUnitValue(1).toString());
  element.setX("10%"s);
  CHECK(element.x().toString() == "10%"s);
  element.setY(2);
  CHECK(element.y().toString() == SvgUnitValue(2).toString());
  element.setY("20mm"s);
  CHECK(element.y().toString() == "20mm"s);
  element.setWidth(3);
  CHECK(element.width().toString() == SvgUnitValue(3).toString());
  element.setWidth(-3);
  CHECK(element.width().toString() == SvgUnitValue(0).toString());
  element.setWidth("30in"s);
  CHECK(element.width().toString() == "30in"s);
  element.setHeight(4);
  CHECK(element.height().toString() == SvgUnitValue(4).toString());
  element.setHeight(-4);
  CHECK(element.height().toString() == SvgUnitValue(0).toString());
  element.setHeight("40px"s);
  CHECK(element.height().toString() == "40px"s);

  element.attributes().add("n1"s, "v1"s);
  CHECK(element.attributes().write() == " n1=\"v1\""s);

  element.appendChild(&element2);
  //  Title, Desc, and Metadata are also children
  // CHECK(element.children().size() == 4);

  //  Copy test
  SvgElement copy(element);
  CHECK(copy.xmlName() == "XmlName"s);
  CHECK(copy.value() == "Value"s);
  CHECK(copy.id() == "Id"s);
  CHECK(copy.title() == "Title"s);
  CHECK(copy.desc() == "Desc"s);
  CHECK(copy.metadata() == "Metadata"s);

  CHECK(copy.x().toString() == "10%"s);
  CHECK(copy.y().toString() == "20mm"s);
  CHECK(copy.width().toString() == "30in"s);
  CHECK(copy.height().toString() == "40px"s);

  SvgElement move(std::move(copy));
  CHECK(move.xmlName() == "XmlName"s);
  CHECK(move.value() == "Value"s);
  CHECK(move.id() == "Id"s);
  CHECK(move.title() == "Title"s);
  CHECK(move.desc() == "Desc"s);
  CHECK(move.metadata() == "Metadata"s);

  CHECK(move.x().toString() == "10%"s);
  CHECK(move.y().toString() == "20mm"s);
  CHECK(move.width().toString() == "30in"s);
  CHECK(move.height().toString() == "40px"s);
}