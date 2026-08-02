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

#include "core/sim/debugger/tvm_interpreter.hpp"

TEST_CASE("tvm::Interpreter: ASYN timestamp decoding", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;

  // These programs are stepped one instruction at a time rather than run to completion, because decoded() only holds
  // the current instruction -- running past the ASYN would overwrite the value under test.
  auto load = [&](tvm::Interpreter &blaster, bits::span<const u8> program) {
    auto *code = mgr->alloc_buffer();
    auto offset = code->append(program);
    blaster.update_ip(code->id(), (u16)offset);
    return code;
  };
  auto timestamp = [](const tvm::Interpreter &blaster) {
    if (std::holds_alternative<tvm::DecodedOp::ASyn>(blaster.decoded()))
      return std::get<tvm::DecodedOp::ASyn>(blaster.decoded()).timestamp;
    else if (std::holds_alternative<tvm::DecodedOp::ISyn>(blaster.decoded()))
      return (u64)std::get<tvm::DecodedOp::ISyn>(blaster.decoded()).delta;
    else throw std::runtime_error("decoded op is not a sync op");
  };

  SECTION("An 8-byte immediate is a full 64-bit timestamp") {
    constexpr auto program = ASyn<1>{}.encode(std::array<u8, 8>{0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF});
    static_assert(program.size() == 12, "opcode word + size word + four payload words");

    tvm::Interpreter blaster(mgr);
    auto *code = load(blaster, program);
    blaster.step();

    CHECK(!blaster.stopped());
    CHECK(timestamp(blaster) == 0xEFCD'AB89'6745'2301ULL);
  }

  SECTION("A narrow unsigned immediate zero-extends") {
    constexpr auto program = ASyn<1>{}.encode(u16(0xFFFF));
    tvm::Interpreter blaster(mgr);
    load(blaster, program);
    blaster.step();

    CHECK(!blaster.stopped());
    CHECK(blaster.regs().MOD1.lo == 2);
    CHECK(timestamp(blaster) == 0x0000'0000'0000'FFFFULL);
  }
 SECTION("A narrow signed immediate sign-extends") {
    constexpr auto program = ISyn<1>{}.encode(u16(0xFFFF));
    tvm::Interpreter blaster(mgr);
    load(blaster, program);
    blaster.step();

    CHECK(!blaster.stopped());
    CHECK(blaster.regs().MOD1.lo == 2);
    CHECK(timestamp(blaster) == 0xFFFF'FFFF'FFFF'FFFFULL);
  }

  SECTION("An immediate wider than 8 bytes is clipped") {
    constexpr auto program =
        ASyn<1>{}.encode(std::array<u8, 12>{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C});
    // Size of the instruction is still 12 bytes
    CHECK(program[2] == 12);

    tvm::Interpreter blaster(mgr);
    load(blaster, program);
    blaster.step();

    CHECK(!blaster.stopped());                             // Extra bytes don't cause execution errors.
    CHECK(blaster.regs().MOD1.lo == 8);                    // clipped in the register
    CHECK(timestamp(blaster) == 0x0807'0605'0403'0201ULL); // trailing bytes are ignored.
  }

  SECTION("With no immediate, the timestamp comes from DP/DS") {
    auto *data = mgr->alloc_buffer();
    auto data_offset = data->append(std::array<u8, 8>{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88});
    auto ldp = LDP<3>(SegmentPair{.hi = data->id().value, .lo = (u16)data_offset}, 8).encode();
    auto asyn = ASyn<0>{}.encode();

    std::vector<u8> program;
    program.insert(program.end(), ldp.begin(), ldp.end());
    program.insert(program.end(), asyn.begin(), asyn.end());

    tvm::Interpreter blaster(mgr);
    load(blaster, program);
    blaster.step(); // LDP
    REQUIRE(blaster.regs().DP.hi == data->id().value);
    REQUIRE(blaster.regs().DS == 8);
    blaster.step(); // ASYN

    CHECK(!blaster.stopped());
    CHECK(timestamp(blaster) == 0x8877'6655'4433'2211ULL);
    // The DP form has no size word to stash, so it leaves the modifier registers alone.
    CHECK(blaster.csrs().M1 == 0);
    CHECK(blaster.csrs().M2 == 0);
  }

  SECTION("A DP-relative ASYN with no data buffer hard stops") {
    // DP starts at {0, 0}, and the manager never hands out buffer ID 0.
    constexpr auto program = ASyn<0>{}.encode();
    tvm::Interpreter blaster(mgr);
    load(blaster, program);
    blaster.step();

    CHECK(blaster.stopped());
    CHECK(blaster.csrs().F == 1); // Hard stop, so a caller iterating programs gives up rather than continuing.
    CHECK(blaster.stop_cause() == StopCause::InvalidDBuffer);
  }
}
