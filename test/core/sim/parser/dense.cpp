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
#include "core/sim/memory/ram/dense.hpp"
#include <catch.hpp>
#include <nlohmann/json.hpp>
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

TEST_CASE("System Parser, Dense, Passes", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  using namespace bits;

  SECTION("with name, compatible") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "ram,dense",
        "basename": "memory",
        "min_offset": 0,
        "max_offset": 1024,
        "fill": 27
      }
      ]
    })j";
    ParsingContext ctx;
    auto s = parse_system(js, ctx);
    REQUIRE(s != nullptr);
    CHECK(s->config().basename == "/");
    CHECK(s->config().fullname == "/");
    CHECK(s->config().compatible == System::compatible);
    auto mem = s->find_relative("memory", "/");
    REQUIRE(mem != nullptr);
    CHECK(mem->config().basename == "memory");
    CHECK(mem->config().fullname == "/memory");
    CHECK(mem->config().compatible == Dense::compatible);
    CHECK(any(mem->type() & Device::Type::MemoryTarget));
    auto casted = mem->capability<Target>();
    REQUIRE(casted != nullptr);
    CHECK(casted->span().lower() == 0);
    CHECK(casted->span().upper() == 1024);
    CHECK(casted->read<u8>(0x0, op_i_std).second == 27);
  }
  SECTION("Fill is optional") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "ram,dense",
        "basename": "memory",
        "min_offset": 0,
        "max_offset": 1024
      }
      ]
    })j";
    ParsingContext ctx;
    auto s = parse_system(js, ctx);
    REQUIRE(s != nullptr);
    auto mem = s->find_relative("memory", "/");
    REQUIRE(mem != nullptr);
    CHECK(mem->config().compatible == Dense::compatible);
    auto casted = mem->capability<Target>();
    REQUIRE(casted != nullptr);
  }
  /*SECTION("coerce string to int") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "ram,dense",
        "basename": "memory",
        "min_offset": "0",
        "max_offset": "1024"
      }
      ]
    })j";
    ParsingContext ctx;
    auto s = parse_system(js, ctx);
    REQUIRE(s != nullptr);
    auto mem = s->find_relative("memory", "/");
    REQUIRE(mem != nullptr);
    CHECK(mem->config().compatible == Dense::compatible);
    auto casted = mem->capability<Target>();
    REQUIRE(casted != nullptr);
    CHECK(casted->span().lower() == 0);
    CHECK(casted->span().upper() == 1024);
  }*/
}

TEST_CASE("System Parser, Dense, Fails", "[scope:core][scope:core.sim][kind:unit][arch:*][!throws]") {
  using namespace bits;

  SECTION("needs min_offset") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "ram,dense",
        "basename": "memory",
        "max_offset": 1024,
        "fill": 0
      }
      ]
    })j";
    ParsingContext ctx;
    REQUIRE_THROWS_AS(parse_system(js, ctx), ParsingError);
  }
  SECTION("needs max_offset") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "ram,dense",
        "basename": "memory",
        "min_offset": 1024,
        "fill": 0
      }
      ]
    })j";
    ParsingContext ctx;
    REQUIRE_THROWS_AS(parse_system(js, ctx), ParsingError);
  }
  // Integer offsets
  SECTION("min_offset must be integer") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "ram,dense",
        "basename": "memory",
        "min_offset": "not an integer",
        "max_offset": 1024,
        "fill": 0
      }
      ]
    })j";
    ParsingContext ctx;
    REQUIRE_THROWS_AS(parse_system(js, ctx), ParsingError);
  }
  SECTION("max_offset must be integer") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "ram,dense",
        "basename": "memory",
        "min_offset": 0,
        "max_offset": "not an integer",
        "fill": 0
      }
      ]
    })j";
    ParsingContext ctx;
    REQUIRE_THROWS_AS(parse_system(js, ctx), ParsingError);
  }
  // fill must be an integer
  SECTION("fill must be integer") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "ram,dense",
        "basename": "memory",
        "min_offset": 0,
        "max_offset": 1024,
        "fill": "not an integer"
      }
      ]
    })j";
    ParsingContext ctx;
    REQUIRE_THROWS_AS(parse_system(js, ctx), ParsingError);
  }
}