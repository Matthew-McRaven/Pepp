#include "sim3/api/traced/memory_path.hpp"
#include <catch.hpp>
#include <zpp_bits.h>

TEST_CASE("Paths", "[scope:sim][kind:int][arch:*]") {
  using namespace sim::api2;
  SECTION("add/find") {
    Paths p;
    CHECK(p.add(0, 10) == 1);
    CHECK(p.add(1, 20) == 2);
    CHECK(p.add(1, 30) == 3);
    CHECK(p.add(2, 30) == 4);
    CHECK(p.add(0, 10) == 1);

    CHECK(p.find(0, 10) == 1);
    CHECK(p.find(1, 20) == 2);
    CHECK(p.find(1, 30) == 3);
    CHECK(p.find(2, 30) == 4);
    CHECK(p.find(0, 0) == 0);
  }
  SECTION("[]") {
    Paths p;
    CHECK(p[0].device == 0);
    CHECK(p[0].current_step == 0);
    CHECK(p[0].previous_step == 0);
    CHECK(p.add(0, 10) == 1);
    CHECK(p[1].device == 10);
    CHECK(p[1].current_step == 1);
    CHECK(p[1].previous_step == 0);
    CHECK(p.add(1, 20) == 2);
    CHECK(p[2].device == 20);
    CHECK(p[2].current_step == 2);
    CHECK(p[2].previous_step == 1);
    CHECK(p.add(2, 30) == 3);
    CHECK(p[3].device == 30);
    CHECK(p[3].current_step == 3);
    CHECK(p[3].previous_step == 2);
    CHECK(p.add(3, 40) == 4);
    CHECK(p[4].device == 40);
    CHECK(p[4].current_step == 4);
    CHECK(p[4].previous_step == 3);
  }
}
