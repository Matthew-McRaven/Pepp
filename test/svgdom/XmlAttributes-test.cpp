#include <catch.hpp>

#include <iostream>
using namespace std::string_literals;

#include "svgdom/XmlAttributes_p.hpp"

TEST_CASE("XmlAttributes_p tests-getter-setter", "[scope:core][scope:core.svgdom.xmlattributes][kind:unit]") {
  //  Test that XmlAttributes class perform correctly

  // std::cout << "From XmlAttributes" << std::endl;
  // Test constructor
  XmlAttributes xml;
  CHECK(xml.size() == 0);

  xml.add("n1"s, "v1"s);
  xml.add("n2"s, "v2"s);
  xml.add("n3"s, "v3"s);
  xml.add("n4"s, "v4"s);

  CHECK(xml.size() == 4);
  CHECK(xml.find("n3") == "v3"s);
  CHECK(xml.find("bad") == ""s); //  Error condition
  CHECK(xml.findValue("v4") == "n4"s);
  CHECK(xml.findValue("bad") == ""s); //  Error condition

  xml.remove("n1"s);
  xml.remove("n4"s);
  xml.remove("n4"s); //  Error condition
  CHECK(xml.size() == 2);

  CHECK(xml.write() == " n2=\"v2\" n3=\"v3\""s);

  xml.clear();
  CHECK(xml.size() == 0);
}

TEST_CASE("XmlAttributes_p tests-copy-move", "[scope:core][scope:core.svgdom.xmlattributes][kind:unit]") {
  //  Test that XmlAttributes class perform correctly

  // std::cout << "From XmlAttributes" << std::endl;
  // Test copy constructor
  XmlAttributes xml;
  CHECK(xml.size() == 0);

  xml.add("n1"s, "v1"s);
  xml.add("n2"s, "v2"s);
  xml.add("n3"s, "v3"s);
  xml.add("n4"s, "v4"s);
  CHECK(xml.size() == 4);

  //  Copy
  XmlAttributes xml2(xml);
  CHECK(xml2.write() == " n1=\"v1\" n2=\"v2\" n3=\"v3\" n4=\"v4\""s);
  CHECK(xml2.size() == 4);

  //  Move
  XmlAttributes xml3(std::move(xml));
  CHECK(xml3.write() == " n1=\"v1\" n2=\"v2\" n3=\"v3\" n4=\"v4\""s);
  CHECK(xml3.size() == 4);
}