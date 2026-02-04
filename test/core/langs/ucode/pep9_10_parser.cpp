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
#include <catch/catch.hpp>

#include "core/arch/pep/uarch/pep.hpp"
#include "core/langs/ucode/pep_parser.hpp"

TEST_CASE("Microassemble 1-byte bus", "[scope:core][scope:core.langs][level:mc2][kind:unit][arch:*][tc2]") {
  using uarch = pepp::tc::arch::Pep9ByteBus;
  using uarch2c = pepp::tc::arch::Pep9WordBusControl;
  using regs = pepp::tc::arch::Pep9Registers;
  using Parser = pepp::tc::parse::MicroParser<uarch, regs>;
  using Parser2c = pepp::tc::parse::MicroParser<uarch2c, regs>;
  SECTION("Integer signals") {
    // Play with spacing on =
    std::string source = "A\t= 1, B = 2, MemRead, MemWrite  , AMux=0, ALU = 1\n";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.empty());
    REQUIRE(result.program.size() == 1);

    auto &line = result.program[0];
    CHECK(line.controls.enables.count() == 6);
    CHECK(line.controls.get(uarch::Signals::A) == 1);
    CHECK(line.controls.get(uarch::Signals::B) == 2);
    CHECK(line.controls.get(uarch::Signals::MemRead) == 1);
    CHECK(line.controls.get(uarch::Signals::MemWrite) == 1);
    CHECK(line.controls.get(uarch::Signals::AMux) == 0);
    CHECK(line.controls.get(uarch::Signals::ALU) == 1);
  }
  SECTION("Now with clocks") {
    std::string source = "C=3;CCk";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.empty());
    REQUIRE(result.program.size() == 1);

    auto &line = result.program[0];
    CHECK(line.controls.enables.count() == 2);
    CHECK(line.controls.get(uarch::Signals::C) == 3);
    CHECK(line.controls.get(uarch::Signals::CCk) == 1);
  }

  SECTION("Microcoded control section") {
    std::string source = "x:C=3;CCk;BR=7, TrueT=6, FalseT=x";
    auto result = Parser2c(std::move(source)).parse();
    CHECK(result.errors.empty());
    REQUIRE(result.program.size() == 1);

    auto &line = result.program[0];
    CHECK(line.controls.enables.count() == 5);
    CHECK(line.controls.get(uarch2c::Signals::C) == 3);
    CHECK(line.controls.get(uarch2c::Signals::CCk) == 1);
    CHECK(line.controls.get(uarch2c::Signals::BR) == 7);
    CHECK(line.controls.get(uarch2c::Signals::TrueT) == 6);
    CHECK(line.symbolDecl.has_value());
    CHECK(result.symbols.at("x") == 0);
  }
  SECTION("Does not need trailing newline") {
    std::string source = "C=3;CCk\nA=15;CCk";
    auto result = Parser(std::move(source)).parse();
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
    std::string source = "C=3;C=3";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
  }
  SECTION("Clocks do not accept values") {
    std::string source = "C=1;CCk=1";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
  }
  SECTION("Enforce max values for signals (1 bit)") {
    std::string source = "AMux=2";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
  }
  SECTION("Enforce max values for signals (5 bits)") {
    {
      std::string source = "C=31";
      auto result = Parser(std::move(source)).parse();
      CHECK(result.errors.size() == 0);
    }
    {
      std::string source = "C=32";
      auto result = Parser(std::move(source)).parse();
      CHECK(result.errors.size() == 1);
    }
  }
  SECTION("No symbol declarations") {
    std::string source = "x:C=0";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
  }
  SECTION("No symbol arguments") {
    std::string source = "C=x";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
  }
  SECTION("No trailing ;") {
    std::string source = "C=0;CCk;";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
  }
  SECTION("Needs , between signals") {
    std::string source = "C=0 A=1";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
  }
  SECTION("No unknown signals") {
    std::string source = "cat=1";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
  }
  SECTION("Signals require value") {
    std::string source = "C=,A=5";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
  }
  SECTION("Disallow undefined symbols") {
    std::string source = "a=5;CCk;TrueT=x";
    auto result = Parser2c(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
  }
  SECTION("Yay formatting") {
    {
      std::string source = "C  =    2, A=1;sck,nCk";
      auto result = Parser(std::move(source)).parse();
      CHECK(result.errors.size() == 0);
      REQUIRE(result.program.size() == 1);
      auto &line = result.program[0];
      auto text = pepp::tc::ir::format<uarch, regs>(line);
      CHECK(text == "A=1, C=2; NCk, SCk");
    }
    {
      std::string source = "x:C  =    2, A=1;;BR=5";
      auto result = Parser2c(std::move(source)).parse();
      CHECK(result.errors.size() == 0);
      REQUIRE(result.program.size() == 1);
      auto &line = result.program[0];
      auto text = pepp::tc::ir::format<uarch2c, regs>(line);
      CHECK(text == "x: A=1, C=2; BR=5");
    }
    {
      std::string source = "A=1    //comments";
      auto result = Parser(std::move(source)).parse();
      CHECK(result.errors.size() == 0);
      REQUIRE(result.program.size() == 1);
      auto &line = result.program[0];
      auto text = pepp::tc::ir::format<uarch, regs>(line);
      CHECK(text == "A=1//comments");
    }
  }
  SECTION("Allow skipping groups") {
    {
      std::string source = "CCk";
      auto result = Parser(std::move(source)).parse();
      CHECK(result.errors.size() == 0);
      REQUIRE(result.program.size() == 1);
      auto &line = result.program[0];
      CHECK(line.controls.enables.count() == 1);
      CHECK(line.controls.get(uarch::Signals::CCk) == 1);
    }
    {
      std::string source = "CCk; Br=5";
      auto result = Parser2c(std::move(source)).parse();
      CHECK(result.errors.size() == 0);
      REQUIRE(result.program.size() == 1);
      auto &line = result.program[0];
      CHECK(line.controls.enables.count() == 2);
      CHECK(line.controls.get(uarch2c::Signals::CCk) == 1);
      CHECK(line.controls.get(uarch2c::Signals::BR) == 5);
    }
    {
      std::string source = "Br=5";
      auto result = Parser2c(std::move(source)).parse();
      CHECK(result.errors.size() == 0);
      REQUIRE(result.program.size() == 1);
      auto &line = result.program[0];
      CHECK(line.controls.enables.count() == 1);
      CHECK(line.controls.get(uarch2c::Signals::BR) == 5);
    }
  }
  SECTION("Valid pre/post") {
    {
      std::string source = "UnitPost: X=5, Mem[0x1]=27";
      auto result = Parser(std::move(source)).parse();
      CHECK(result.errors.size() == 0);
      REQUIRE(result.program.size() == 1);
      auto &line = result.program[0];
      CHECK(line.controls.enables.count() == 0);
      CHECK(line.type == pepp::tc::ir::Line<uarch, regs>::Type::Post);
      CHECK(line.tests.size() == 2);
    }
    {
      std::string source = "UnitPre: IR=0x123456, Mem[0x1]=0xFE";
      auto result = Parser(std::move(source)).parse();
      CHECK(result.errors.size() == 0);
      REQUIRE(result.program.size() == 1);
      auto &line = result.program[0];
      CHECK(line.controls.enables.count() == 0);
      CHECK(line.type == pepp::tc::ir::Line<uarch, regs>::Type::Pre);
      CHECK(line.tests.size() == 2);
    }
  }
  SECTION("Extract microcode lines") {
    std::string source = "UnitPost: X=5, Mem[0x1]=27\n//test comment\nA=7\n\nB=3";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 0);
    CHECK(result.program.size() == 5);
    auto code = pepp::tc::parse::microcodeFor<uarch, regs>(result);
    REQUIRE(code.size() == 2);
    CHECK(code[0].A == 7);
    CHECK(code[1].B == 3);
  }

  SECTION("Test missing ,") {
    std::string source = "UnitPost: X=5 Mem[0x1]=27";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
    CHECK(result.program.size() == 0);
  }
  SECTION("Test missing =") {
    std::string source = "UnitPost: X 5";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
    CHECK(result.program.size() == 0);
  }
  SECTION("Test no symbols") {
    std::string source = "UnitPost: x:X=5";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
    CHECK(result.program.size() == 0);
  }
  SECTION("Test missing number") {
    std::string source = "UnitPost: X=X";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
    CHECK(result.program.size() == 0);
  }
  SECTION("Test memory address out-of-range") {
    std::string source = "UnitPost: Mem[0x10000]=0";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
    CHECK(result.program.size() == 0);
  }
  SECTION("Test memory value out-of-range") {
    std::string source = "UnitPost: Mem[0xFFFF]=0x10000";
    auto result = Parser(std::move(source)).parse();
    CHECK(result.errors.size() == 1);
    CHECK(result.program.size() == 0);
  }
  SECTION("Test register value out-of-range") {
    {
      std::string good_source = "UnitPost: T1=0xFF";
      auto result = Parser(std::move(good_source)).parse();
      CHECK(result.errors.size() == 0);
      CHECK(result.program.size() == 1);
      std::string bad_source = "UnitPost: T1=0x100";
      result = Parser(std::move(bad_source)).parse();
      CHECK(result.errors.size() == 1);
      CHECK(result.program.size() == 0);
    }
    {
      std::string good_source = "UnitPost: X=0xFFFF";
      auto result = Parser(std::move(good_source)).parse();
      CHECK(result.errors.size() == 0);
      CHECK(result.program.size() == 1);
      std::string bad_source = "UnitPost: X=0x10000";
      result = Parser(std::move(bad_source)).parse();
      CHECK(result.errors.size() == 1);
      CHECK(result.program.size() == 0);
    }
    {
      std::string good_source = "UnitPost: IR=0xFFFFFF";
      auto result = Parser(std::move(good_source)).parse();
      CHECK(result.errors.size() == 0);
      CHECK(result.program.size() == 1);
      std::string bad_source = "UnitPost: IR=0x1000000";
      result = Parser(std::move(bad_source)).parse();
      CHECK(result.errors.size() == 1);
      CHECK(result.program.size() == 0);
    }
  }
}
