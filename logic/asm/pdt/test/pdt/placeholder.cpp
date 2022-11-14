#include "catch.hpp"
#include "pdt/placeholder.hpp"
TEST_CASE("placeholder") {

  SECTION("placeholder") {
    placeholder();
    REQUIRE(true == true);
  }

}