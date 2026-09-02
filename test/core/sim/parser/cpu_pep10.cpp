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
#include "core/sim/cores/cpu/pep/pep_isa.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

TEST_CASE("System Parser,  Pep/10 ISA3 CPU, Passes", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  using namespace bits;

  SECTION("with name, compatible") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "ram,dense",
        "basename": "memory",
        "min_offset": 0,
        "max_offset": 1024
      },
      {
        "compatible": "cpu,pep,isa3",
        "basename": "cpu",
        "target": "/memory"
      }
      ]
    })j";

    auto s = parse_system(js);
    REQUIRE(s != nullptr);
    // Contains system root, memory, cpu, csrs, regs
    REQUIRE(std::distance(s->root()->begin(), s->root()->end()) == 5);
    s->initialize();
    CHECK(s->config().basename == "/");
    CHECK(s->config().fullname == "/");
    CHECK(s->config().compatible == System::compatible);
    // Contains system root, memory, cpu, csrs, regs
    REQUIRE(std::distance(s->root()->begin(), s->root()->end()) == 5);
    auto mem = s->find_relative("memory", "/");
    REQUIRE(mem != nullptr);
    CHECK(mem->config().basename == "memory");
    CHECK(mem->config().fullname == "/memory");
    CHECK(mem->config().compatible == Dense::compatible);
    CHECK(any(mem->type() & Device::Type::MemoryTarget));
    auto cpu = s->find_relative("cpu", "/");
    REQUIRE(cpu != nullptr);
    CHECK(cpu->config().basename == "cpu");
    CHECK(cpu->config().fullname == "/cpu");
    CHECK(cpu->config().compatible == PepISA3CPU::compatible);
    auto regs = s->find_relative("regs", cpu->config().fullname);
    REQUIRE(regs != nullptr);
    CHECK(regs->config().basename == "regs");
    CHECK(regs->config().fullname == "/cpu/regs");
    auto csrs = s->find_relative("csrs", cpu->config().fullname);
    REQUIRE(csrs != nullptr);
    CHECK(csrs->config().basename == "csrs");
    CHECK(csrs->config().fullname == "/cpu/csrs");
  }
  SECTION("serialization") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "cpu,pep,isa3",
        "basename": "cpu",
        "target": "memory"
      }
      ]
    })j";

    auto s = parse_system(js);
    REQUIRE(s != nullptr);
    auto cpu = s->find_relative("cpu", "/");
    REQUIRE(cpu != nullptr);
    nlohmann::json obj;
    cpu->serializer()->serialize(obj, s.get(), cpu);
    CHECK(obj["compatible"] == PepISA3CPU::compatible);
    CHECK(obj["basename"] == "cpu");
    CHECK(obj["target"] == "memory");
    CHECK(obj["isa"] == "pep10");
    CHECK(!obj.contains("children"));
  }
}

TEST_CASE("System Parser, Pep/10 ISA3 CPU, Fails", "[scope:core][scope:core.sim][kind:unit][arch:*][!throws]") {
  using namespace bits;

  SECTION("needs target") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "cpu,pep,isa3",
        "basename": "cpu"
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }
  SECTION("target is string") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "cpu,pep,isa3",
        "basename": "cpu",
				"target": 15
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }
  SECTION("isa is string") {
    static const char *js = R"j({
      "children": [
      {
        "compatible": "cpu,pep,isa3",
        "basename": "cpu",
        "target":"memory",
				"isa": 15
      }
      ]
    })j";

    REQUIRE_THROWS_AS(parse_system(js), ParsingError);
  }
}