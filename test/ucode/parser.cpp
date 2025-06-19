/*
 * Copyright (c) 2023-2025 J. Stanley Warford, Matthew McRaven
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

#include "toolchain/ucode/parser.hpp"
#include "toolchain/ucode/uarch.hpp"

TEST_CASE("Microassemble 1-byte bus", "[scope:ucode][kind:unit][arch:*]") {
  using uarch = pepp::ucode::Pep9ByteBus;
  using uarch2c = pepp::ucode::Pep9WordBusControl;
  SECTION("Integer signals") {
    // Play with spacing on =
    QString source = "A\t= 1, B = 2, MemRead =1, MemWrite= 0  , AMux=0, ALU = 1\n";
    auto result = pepp::ucode::parse<uarch>(source);
    CHECK(result.errors.empty());
    REQUIRE(result.program.size() == 2);

    auto &line = result.program[0];
    CHECK(line.controls.enables.count() == 6);
    CHECK(line.controls.get(uarch::Signals::A) == 1);
    CHECK(line.controls.get(uarch::Signals::B) == 2);
    CHECK(line.controls.get(uarch::Signals::MemRead) == 1);
    CHECK(line.controls.get(uarch::Signals::MemWrite) == 0);
    CHECK(line.controls.get(uarch::Signals::AMux) == 0);
    CHECK(line.controls.get(uarch::Signals::ALU) == 1);
  }
  SECTION("Now with clocks") {
    QString source = "C=3;CCk";
    auto result = pepp::ucode::parse<uarch>(source);
    CHECK(result.errors.empty());
    REQUIRE(result.program.size() == 1);

    auto &line = result.program[0];
    CHECK(line.controls.enables.count() == 2);
    CHECK(line.controls.get(uarch::Signals::C) == 3);
    CHECK(line.controls.get(uarch::Signals::CCk) == 1);
  }

  SECTION("Microcoded control section") {
    QString source = "x:C=3;CCk;BR=7, TrueT=6, FalseT=x";
    auto result = pepp::ucode::parse<uarch2c>(source);
    CHECK(result.errors.empty());
    REQUIRE(result.program.size() == 1);

    auto &line = result.program[0];
    CHECK(line.controls.enables.count() == 5);
    CHECK(line.controls.get(uarch2c::Signals::C) == 3);
    CHECK(line.controls.get(uarch2c::Signals::CCk) == 1);
    CHECK(line.controls.get(uarch2c::Signals::BR) == 7);
    CHECK(line.controls.get(uarch2c::Signals::TrueT) == 6);
    CHECK(line.symbolDecl.has_value());
    CHECK(result.symbols.value("x", -1) == 0);
  }
  SECTION("Does not need trailing newline") {
    QString source = "C=3;CCk\nA=15;CCk";
    auto result = pepp::ucode::parse<uarch>(source);
    CHECK(result.errors.empty());
    REQUIRE(result.program.size() == 2);

    auto &line = result.program[0];
    CHECK(line.controls.enables.count() == 2);
    CHECK(line.controls.get(uarch::Signals::C) == 3);
    CHECK(line.controls.get(uarch::Signals::CCk) == 1);

    line = result.program[1];
    CHECK(line.controls.enables.count() == 2);
    CHECK(line.controls.get(uarch::Signals::A) == 15);
    CHECK(line.controls.get(uarch::Signals::CCk) == 1);
  }
  SECTION("No duplicate signals") {
    QString source = "C=3;C=3";
    auto result = pepp::ucode::parse<uarch>(source);
    CHECK(result.errors.size() == 1);
  }
  SECTION("Clocks do not accept values") {
    QString source = "C=1;CCk=1";
    auto result = pepp::ucode::parse<uarch>(source);
    CHECK(result.errors.size() == 1);
  }
  SECTION("Enforce max values for signals (1 bit)") {
    QString source = "AMux=2";
    auto result = pepp::ucode::parse<uarch>(source);
    CHECK(result.errors.size() == 1);
  }
  SECTION("Enforce max values for signals (5 bits)") {
    {
      QString source = "C=31";
      auto result = pepp::ucode::parse<uarch>(source);
      CHECK(result.errors.size() == 0);
    }
    {
      QString source = "C=32";
      auto result = pepp::ucode::parse<uarch>(source);
      CHECK(result.errors.size() == 1);
    }
  }
  SECTION("No symbol declarations") {
    QString source = "x:C=0";
    auto result = pepp::ucode::parse<uarch>(source);
    CHECK(result.errors.size() == 1);
  }
  SECTION("No symbol arguments") {
    QString source = "C=x";
    auto result = pepp::ucode::parse<uarch>(source);
    CHECK(result.errors.size() == 1);
  }
  SECTION("No trailing ;") {
    QString source = "C=0;CCk;";
    auto result = pepp::ucode::parse<uarch>(source);
    CHECK(result.errors.size() == 1);
  }
  SECTION("Needs , between signals") {
    QString source = "C=0 A=1";
    auto result = pepp::ucode::parse<uarch>(source);
    CHECK(result.errors.size() == 1);
  }
  SECTION("No unknown signals") {
    QString source = "cat=1";
    auto result = pepp::ucode::parse<uarch>(source);
    CHECK(result.errors.size() == 1);
  }
  SECTION("Signals require value") {
    QString source = "C=,A=5";
    auto result = pepp::ucode::parse<uarch>(source);
    CHECK(result.errors.size() == 1);
  }
  SECTION("Yay formatting") {
    {
      QString source = "C  =    2, A=1;sck,nCk";
      auto result = pepp::ucode::parse<uarch>(source);
      CHECK(result.errors.size() == 0);
      REQUIRE(result.program.size() == 1);
      auto &line = result.program[0];
      auto text = pepp::ucode::format<uarch>(line);
      CHECK(text.toStdString() == "A=1, C=2; NCk, SCk");
    }
    {
      QString source = "x:C  =    2, A=1;;BR=5";
      auto result = pepp::ucode::parse<uarch2c>(source);
      CHECK(result.errors.size() == 0);
      REQUIRE(result.program.size() == 1);
      auto &line = result.program[0];
      auto text = pepp::ucode::format<uarch2c>(line);
      CHECK(text.toStdString() == "x: A=1, C=2; BR=5");
    }
    {
      QString source = "A=1    //comments";
      auto result = pepp::ucode::parse<uarch>(source);
      CHECK(result.errors.size() == 0);
      REQUIRE(result.program.size() == 1);
      auto &line = result.program[0];
      auto text = pepp::ucode::format<uarch>(line);
      CHECK(text.toStdString() == "A=1//comments");
    }
  }
}
