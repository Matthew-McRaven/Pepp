#include <catch.hpp>

using namespace std::string_literals;

#include "svgdom/SvgParser.hpp"

TEST_CASE("FunctionParser tests", "[scope:core][scope:core.svgdom.functionparser][kind:unit]") {
  //  Test that FunctionParser class perform correctly

  // std::cout << "From FunctionParser" << std::endl;

  std::string_view test = "";
  {
    const auto results = FunctionParser::parse(test);
    REQUIRE(!results.has_value());
  }

  //  Most basic-no spaces, 1 argument
  test = "rotate(25.1)";
  {
    const auto results = FunctionParser::parse(test);
    REQUIRE(results.has_value());
    CHECK((*results).size() == 1);

    auto result = (*results).at(0);
    CHECK(result.first == "rotate"s);
    CHECK(result.second.size() == 1);
    CHECK(result.second.at(0) == 25.1);
  }
  //  No spaces, negative
  test = "rotate(-12.3)";
  {
    const auto results = FunctionParser::parse(test);
    REQUIRE(results.has_value());
    CHECK((*results).size() == 1);

    auto result = (*results).at(0);
    CHECK(result.first == "rotate"s);
    CHECK(result.second.size() == 1);
    CHECK(result.second.at(0) == -12.3);
  }

  //  No spaces, 2 arguments with signs
  test = "rotate(-12.3 +32.1)";
  {
    const auto results = FunctionParser::parse(test);
    REQUIRE(results.has_value());
    CHECK((*results).size() == 1);

    auto result = (*results).at(0);
    CHECK(result.first == "rotate"s);
    CHECK(result.second.size() == 2);
    CHECK(result.second.at(0) == -12.3);
    CHECK(result.second.at(1) == 32.1);
  }
  //  No spaces, 3 arguments with commas
  test = "rotate(-12.3, +32.1 ,234 )";
  {
    const auto results = FunctionParser::parse(test);
    REQUIRE(results.has_value());
    CHECK((*results).size() == 1);

    auto result = (*results).at(0);
    CHECK(result.first == "rotate"s);
    CHECK(result.second.size() == 3);
    CHECK(result.second.at(0) == -12.3);
    CHECK(result.second.at(1) == 32.1);
    CHECK(result.second.at(2) == 234);
  }

  //  2 functions, some spaces
  test = "rotate(-12.3, +32.1 ,234 ) skewX( 678 )";
  {
    const auto results = FunctionParser::parse(test);
    REQUIRE(results.has_value());
    CHECK((*results).size() == 2);

    auto result = (*results).at(0);
    CHECK(result.first == "rotate"s);
    CHECK(result.second.size() == 3);
    CHECK(result.second.at(0) == -12.3);
    CHECK(result.second.at(1) == 32.1);
    CHECK(result.second.at(2) == 234);

    result = (*results).at(1);
    CHECK(result.first == "skewX"s);
    CHECK(result.second.size() == 1);
    CHECK(result.second.at(0) == 678);
  }
  //  5 functions, torture test
  test = "skewY(876) translate ( 45,54)  scale( 96, 69 ) rotate(-12.3, +32.1 ,234 ) skewX( 678 )";
  {
    const auto results = FunctionParser::parse(test);
    REQUIRE(results.has_value());
    CHECK((*results).size() == 5);

    auto result = (*results).at(0);
    CHECK(result.first == "skewY"s);
    CHECK(result.second.size() == 1);
    CHECK(result.second.at(0) == 876);

    result = (*results).at(1);
    CHECK(result.first == "translate"s);
    CHECK(result.second.size() == 2);
    CHECK(result.second.at(0) == 45);
    CHECK(result.second.at(1) == 54);

    result = (*results).at(2);
    CHECK(result.first == "scale"s);
    CHECK(result.second.size() == 2);
    CHECK(result.second.at(0) == 96);
    CHECK(result.second.at(1) == 69);

    result = (*results).at(3);
    CHECK(result.first == "rotate"s);
    CHECK(result.second.size() == 3);
    CHECK(result.second.at(0) == -12.3);
    CHECK(result.second.at(1) == 32.1);
    CHECK(result.second.at(2) == 234);

    result = (*results).at(4);
    CHECK(result.first == "skewX"s);
    CHECK(result.second.size() == 1);
    CHECK(result.second.at(0) == 678);
  }
}
