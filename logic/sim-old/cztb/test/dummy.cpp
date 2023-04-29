#include <catch.hpp>

TEST_CASE("placeholder") {
  SECTION("stays alive") {
    REQUIRE(1==1);
  }
}
