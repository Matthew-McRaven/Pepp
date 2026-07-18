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
#include "core/sim/memory/io/mm.hpp"
#include <catch.hpp>
#include <nlohmann/json.hpp>
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

TEST_CASE("System Parser, MemoryMappedRegister, Passes", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  using namespace bits;

  SECTION("with name, compatible") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "io,reg",
        "basename": "memory",
        "offset": 0,
        "fill": 27
      }
      ]
    })j";

    auto s = parse_system(js);
    REQUIRE(s != nullptr);
    CHECK(s->config().basename == "/");
    CHECK(s->config().fullname == "/");
    CHECK(s->config().compatible == System::compatible);
    auto mem = s->find_relative("memory", "/");
    REQUIRE(mem != nullptr);
    CHECK(mem->config().basename == "memory");
    CHECK(mem->config().fullname == "/memory");
    CHECK(mem->config().compatible == MemoryMappedRegister::compatible);
    CHECK(any(mem->type() & Device::Type::MemoryTarget));
    auto casted = mem->capability<Target>();
    REQUIRE(casted != nullptr);
    CHECK(casted->span().lower() == 0);
    CHECK(casted->span().upper() == 0);
  }
  SECTION("Fill is optional") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "io,reg",
        "basename": "memory",
        "offset": 27
      }
      ]
    })j";

    auto s = parse_system(js);
    REQUIRE(s != nullptr);
    auto mem = s->find_relative("memory", "/");
    REQUIRE(mem != nullptr);
    CHECK(mem->config().compatible == MemoryMappedRegister::compatible);
    auto casted = mem->capability<Target>();
    REQUIRE(casted != nullptr);
    CHECK(casted->span().lower() == 27);
    CHECK(casted->span().upper() == 27);
  }
  SECTION("Non-default faily policy") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "io,reg",
        "basename": "memory",
        "offset": 27,
				"fail_policy": "yield_default"
      }
      ]
    })j";

    auto s = parse_system(js);
    REQUIRE(s != nullptr);
    auto mem = s->find_relative("memory", "/");
    auto casted = dynamic_cast<MemoryMappedRegister *>(mem);
    REQUIRE(casted != nullptr);
    CHECK(((MemoryMappedRegister::Configuration &)casted->config()).fail_policy == FailPolicy::YieldDefaultValue);
  }
  SECTION("direction: input") {
    using namespace bits;
    using Direction = MemoryMappedRegister::IODirection;
    static const char *js = R"j({
      "children": [
      {
        "compatible": "io,reg",
        "basename": "memory",
        "offset": 27,
        "direction" :"in"
      }
      ]
    })j";

    auto s = parse_system(js);
    REQUIRE(s != nullptr);
    auto mem = s->find_relative("memory", "/");
    auto casted = dynamic_cast<MemoryMappedRegister *>(mem);
    REQUIRE(casted != nullptr);
    CHECK(any(((MemoryMappedRegister::Configuration &)casted->config()).direction & Direction::Input));
  }
  SECTION("direction: output") {
    using namespace bits;
    using Direction = MemoryMappedRegister::IODirection;
    static const char *js = R"j({
      "children": [
      {
        "compatible": "io,reg",
        "basename": "memory",
        "offset": 27,
        "direction" :"out"
      }
      ]
    })j";

    auto s = parse_system(js);
    REQUIRE(s != nullptr);
    auto mem = s->find_relative("memory", "/");
    auto casted = dynamic_cast<MemoryMappedRegister *>(mem);
    REQUIRE(casted != nullptr);
    CHECK(any(((MemoryMappedRegister::Configuration &)casted->config()).direction & Direction::Output));
  }
  SECTION("direction: output") {
    using namespace bits;
    using Direction = MemoryMappedRegister::IODirection;
    static const char *js = R"j({
      "children": [
      {
        "compatible": "io,reg",
        "basename": "memory",
        "offset": 27,
        "direction" :"inout"
      }
      ]
    })j";

    auto s = parse_system(js);
    REQUIRE(s != nullptr);
    auto mem = s->find_relative("memory", "/");
    auto casted = dynamic_cast<MemoryMappedRegister *>(mem);
    REQUIRE(casted != nullptr);
    CHECK(any(((MemoryMappedRegister::Configuration &)casted->config()).direction & Direction::Output));
    CHECK(any(((MemoryMappedRegister::Configuration &)casted->config()).direction & Direction::Input));
  }
  SECTION("serialization") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "io,reg",
        "basename": "memory",
        "offset": 1
      }
      ]
    })j";

    auto s = parse_system(js);
    REQUIRE(s != nullptr);
    auto mem = s->find_relative("memory", "/");
    REQUIRE(mem != nullptr);
    nlohmann::json obj;
    mem->serializer()->serialize(obj, s.get(), mem);
    CHECK(obj["compatible"] == MemoryMappedRegister::compatible);
    CHECK(obj["basename"] == "memory");
    CHECK(obj["offset"] == 1);
    CHECK(obj["direction"] == "none");
  }
}

TEST_CASE("System Parser, MemoryMappedRegister, Fails", "[scope:core][scope:core.sim][kind:unit][arch:*][!throws]") {
  using namespace bits;

  SECTION("needs offset") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "io,reg",
        "basename": "memory",
        "fill": 0
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }
  // Integer offsets
  SECTION("offset must be integer") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "io,reg",
        "basename": "memory",
        "offset": "not an integer",
        "fill": 0
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }

  // fill must be an integer
  SECTION("fill must be integer") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "io,reg",
        "basename": "memory",
        "offset": 27,
        "fill": "not an integer"
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }
}