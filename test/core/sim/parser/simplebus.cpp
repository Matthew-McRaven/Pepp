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
#include "core/sim/memory/bus/simplebus.hpp"
#include <catch.hpp>
#include <nlohmann/json.hpp>
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

TEST_CASE("System Parser, SimpleBus, Passes", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  using namespace bits;

  SECTION("with name, compatible") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "bus,simple",
        "basename": "memory",
        "min_offset": 0,
        "max_offset": 1024,
        "fill": 27,
        "mappings":[]
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
    CHECK(mem->config().compatible == SimpleBus::compatible);
    CHECK(any(mem->type() & Device::Type::MemoryTarget));
    auto casted = mem->capability<Target>();
  }
  SECTION("Fill is optional") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "bus,simple",
        "basename": "memory",
        "min_offset": 0,
        "max_offset": 1024,
        "mappings": []
      }
      ]
    })j";

    auto s = parse_system(js);
    REQUIRE(s != nullptr);
    auto mem = s->find_relative("memory", "/");
    REQUIRE(mem != nullptr);
    CHECK(mem->config().compatible == SimpleBus::compatible);
    auto casted = mem->capability<Target>();
    REQUIRE(casted != nullptr);
  }
  SECTION("coerce string to int") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "bus,simple",
        "basename": "memory",
        "min_offset": "0b000001",
        "max_offset": "1024",
        "mappings": []
      }
      ]
    })j";

    auto s = parse_system(js);
    REQUIRE(s != nullptr);
    auto mem = s->find_relative("memory", "/");
    REQUIRE(mem != nullptr);
    CHECK(mem->config().compatible == SimpleBus::compatible);
    auto casted = mem->capability<Target>();
    REQUIRE(casted != nullptr);
    CHECK(casted->span().lower() == 1);
    CHECK(casted->span().upper() == 1024);
  }
  SECTION("with one child") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "ram,dense",
        "basename": "memory",
        "min_offset": "0",
        "max_offset": "1000",
        "fill": 13
      },
      {
        "compatible": "bus,simple",
        "basename": "bus",
        "min_offset": 0,
        "max_offset": 1024,
        "fill": 27,
        "fail_policy": "yield_default",
        "mappings": [{
            "source_min_offset": 0,
            "source_max_offset": 20,
            "target_offset": 80,
            "target": "/memory"
        }]
      }
      ]
    })j";

    auto s = parse_system(js);
    s->initialize();
    REQUIRE(s != nullptr);
    auto mem = s->find_relative("memory", "/");
    REQUIRE(mem != nullptr);
    CHECK(mem->config().compatible == Dense::compatible);

    auto maybe_bus = s->find_relative("bus", "/");
    REQUIRE(maybe_bus != nullptr);
    CHECK(maybe_bus->config().compatible == SimpleBus::compatible);

    auto bus = dynamic_cast<SimpleBus *>(maybe_bus);
    REQUIRE(bus != nullptr);
    CHECK(bus->span().lower() == 0);
    CHECK(bus->span().upper() == 1024);
    CHECK(bus->mappings().size() == 1);
    auto first_mapping = bus->mappings().front();
    CHECK(first_mapping.source_span.lower() == 0);
    CHECK(first_mapping.source_span.upper() == 20);
    CHECK(first_mapping.target_offset == 80);
    CHECK(first_mapping.target == "/memory");
    auto casted = bus->capability<Target>();
    REQUIRE(casted != nullptr);
    // Read to mapped address. Read default value of target.
    CHECK(casted->read<u8>(0x10, op_i_std).second == 13);

    // Serialization
    nlohmann::json obj;
    bus->serializer()->serialize(obj, s.get(), bus);
    CHECK(obj["compatible"] == SimpleBus::compatible);
    CHECK(obj["basename"] == "bus");
    CHECK(obj["min_offset"] == 0);
    CHECK(obj["max_offset"] == 1024);
    CHECK(obj["fail_policy"] == "yield_default");
    CHECK(obj["mappings"][0]["source_min_offset"] == 0);
    CHECK(obj["mappings"][0]["source_max_offset"] == 20);
    CHECK(obj["mappings"][0]["target_offset"] == 80);
    CHECK(obj["mappings"][0]["target"] == "/memory");
  }
}

TEST_CASE("System Parser, SimpleBus, Fails", "[scope:core][scope:core.sim][kind:unit][arch:*][!throws]") {
  using namespace bits;

  SECTION("needs min_offset") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "bus,simple",
        "mappings": [],
        "basename": "memory",
        "max_offset": 1024,
        "fill": 0
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }
  SECTION("needs max_offset") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "bus,simple",
        "mappings": [],
        "basename": "memory",
        "min_offset": 1024,
        "fill": 0
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }
  // Integer offsets
  SECTION("min_offset must be integer") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "bus,simple",
        "mappings": [],
        "basename": "memory",
        "min_offset": "not an integer",
        "max_offset": 1024,
        "fill": 0
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }
  SECTION("max_offset must be integer") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "bus,simple",
        "mappings": [],
        "basename": "memory",
        "min_offset": 0,
        "max_offset": "not an integer",
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
        "compatible": "bus,simple",
        "mappings": [],
        "basename": "memory",
        "min_offset": 0,
        "max_offset": 1024,
        "fill": "not an integer"
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }
  SECTION("bad octal") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "bus,simple",
        "mappings": [],
        "basename": "memory",
        "min_offset": 0,
        "max_offset": 1024,
        "fill": "0o88"
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }
  SECTION("out-of-range fill") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "bus,simple",
        "mappings": [],
        "basename": "memory",
        "min_offset": 0,
        "max_offset": 1024,
        "fill": "0x100"
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }
  SECTION("negative in unsigned slot") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "bus,simple",
        "mappings": [],
        "basename": "memory",
        "min_offset": -100,
        "max_offset": 1024,
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }

  SECTION("mapping field is required") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "bus,simple",
        "basename": "memory",
        "min_offset": 0,
        "max_offset": 1024,
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }
  SECTION("mappings must be arrays") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "bus,simple",
        "mapping": "cat",
        "basename": "memory",
        "min_offset": 0,
        "max_offset": 1024,
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }
}