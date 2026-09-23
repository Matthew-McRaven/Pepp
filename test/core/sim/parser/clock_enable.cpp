/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <catch.hpp>
#include <nlohmann/json.hpp>
#include <variant>
#include "core/sim/clocktree.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

namespace {
using Enable = pepp::ClockEnable;

// A 2-byte state register at /ctl and an ideal clock at /clk carrying the given enable.
std::unique_ptr<System> with_enable(const std::string &enable) {
  return parse_system(R"({"children": [
    {"compatible": "io,state", "basename": "ctl", "offset": 0, "width": 2},
    {"compatible": "clock,ideal", "basename": "clk", "period": 10, "enable": )" +
                      enable + "}]}");
}

const std::optional<Enable::Configuration> &enable_of(System &sys) {
  auto *clk = dynamic_cast<pepp::IdealClock *>(sys.find_absolute("/clk"));
  REQUIRE(clk != nullptr);
  return clk->casted_config().enable;
}
} // namespace

TEST_CASE("System Parser, clock enable, Passes", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  SECTION("Absent or null") {
    auto s = parse_system(R"({"children": [{"compatible": "clock,ideal", "basename": "clk", "period": 10}]})");
    CHECK(!enable_of(*s).has_value());
    CHECK(!enable_of(*with_enable("null")).has_value());
  }
  SECTION("disable_on_write spans whole source by default") {
    auto s = with_enable(R"({"source": "/ctl", "mode": "disable_on_write"})");
    const auto &e = enable_of(*s);
    REQUIRE(e.has_value());
    CHECK(e->source == "/ctl");
    CHECK(!e->span.has_value());
    CHECK(std::holds_alternative<Enable::DisableOnWrite>(e->mode));
  }
  SECTION("EnableWhenAny") {
    auto s = with_enable(R"({"source": "/ctl", "mode": "enable_when", "offset": 1})");
    const auto &e = enable_of(*s);
    REQUIRE(e.has_value());
    CHECK(e->span == AddressSpan(1, 1));
    auto *any = std::get_if<Enable::EnableWhenAny>(&e->mode);
    REQUIRE(any != nullptr);
    CHECK(any->mask == ~u64{0});
    CHECK(any->order == bits::Order::LittleEndian);
  }
  SECTION("EnableWhenEqual") {
    auto s = with_enable(
        R"({"source": "ctl", "mode": "enable_when", "offset": "0x0", "width": "2", "mask": "0b11", "match": "0x2",
            "order": "big"})");
    const auto &e = enable_of(*s);
    REQUIRE(e.has_value());
    CHECK(e->source == "ctl");
    CHECK(e->span == AddressSpan(0, 1));
    auto *eq = std::get_if<Enable::EnableWhenEqual>(&e->mode);
    REQUIRE(eq != nullptr);
    CHECK(eq->mask == 0b11);
    CHECK(eq->match == 2);
    CHECK(eq->order == bits::Order::BigEndian);
  }
}

TEST_CASE("System Parser, clock enable, Fails", "[scope:core][scope:core.sim][kind:unit][arch:*][!throws]") {
  SECTION("Not an object") { CHECK_THROWS_AS(with_enable(R"("/ctl")"), ParsingError); }
  SECTION("Missing source") { CHECK_THROWS_AS(with_enable(R"({"mode": "disable_on_write"})"), ParsingError); }
  SECTION("Missing mode") { CHECK_THROWS_AS(with_enable(R"({"source": "/ctl"})"), ParsingError); }
  SECTION("Unknown mode") {
    CHECK_THROWS_AS(with_enable(R"({"source": "/ctl", "mode": "sometimes"})"), ParsingError);
  }
  SECTION("Unknown order") {
    CHECK_THROWS_AS(with_enable(R"({"source": "/ctl", "mode": "enable_when", "order": "middle"})"), ParsingError);
  }
  SECTION("Width of 0") {
    CHECK_THROWS_AS(with_enable(R"({"source": "/ctl", "mode": "enable_when", "offset": 0, "width": 0})"),
                    ParsingError);
  }
  SECTION("Width without offset") {
    CHECK_THROWS_AS(with_enable(R"({"source": "/ctl", "mode": "enable_when", "width": 1})"), ParsingError);
  }
  SECTION("Range past the end of the address space") {
    CHECK_THROWS_AS(with_enable(R"({"source": "/ctl", "mode": "enable_when", "offset": "0xFFFFFFFF", "width": 2})"),
                    ParsingError);
  }
  SECTION("disable_on_write with value fields") {
    for (auto field : {R"("mask": 1)", R"("match": 1)", R"("order": "big")"})
      CHECK_THROWS_AS(with_enable(std::string(R"({"source": "/ctl", "mode": "disable_on_write", )") + field + "}"),
                      ParsingError);
  }
}

TEST_CASE("System Parser, clock enable, Fails during initialize",
          "[scope:core][scope:core.sim][kind:unit][arch:*][!throws]") {
  SECTION("Source does not exist") {
    auto s = with_enable(R"({"source": "/nope", "mode": "disable_on_write"})");
    CHECK_THROWS(s->initialize());
  }
  SECTION("Source does not raise events") {
    auto s = parse_system(R"({"children": [
      {"compatible": "ram,dense", "basename": "ram", "min_offset": 0, "max_offset": 1},
      {"compatible": "clock,ideal", "basename": "clk", "period": 10,
       "enable": {"source": "/ram", "mode": "disable_on_write"}}
    ]})");
    CHECK_THROWS(s->initialize());
  }
  SECTION("Offset outside the source") {
    auto s = with_enable(R"({"source": "/ctl", "mode": "enable_when", "offset": 2})");
    CHECK_THROWS(s->initialize());
  }
}
