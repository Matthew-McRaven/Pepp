/*
 * /Copyright (c) 2023-2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "core/arch/pep/isa/pep_shared_ops.hpp"
#include "core/ds/string_compare.hpp"

namespace isa::detail::pep9 {
enum class Mnemonic {
  STOP = 0x0,
  RET = 0x1,
  RETTR = 0x2,
  MOVSPA = 0x3,
  MOVFLGA = 0x4,
  MOVAFLG = 0x5,

  NOTA = 0x06,
  NOTX = 0x07,
  NEGA = 0x08,
  NEGX = 0x09,
  ASLA = 0x0A,
  ASLX = 0x0B,
  ASRA = 0x0C,
  ASRX = 0x0D,
  ROLA = 0x0E,
  ROLX = 0x0F,
  RORA = 0x10,
  RORX = 0x11,

  BR = 0x12,
  BRLE = 0x14,
  BRLT = 0x16,
  BREQ = 0x18,
  BRNE = 0x1A,
  BRGE = 0x1C,
  BRGT = 0x1E,
  BRV = 0x20,
  BRC = 0x22,
  CALL = 0x24,

  // Trap opcodes
  NOP0 = 0x26,
  NOP1 = 0x27,
  NOP = 0x28,
  DECI = 0x30,
  DECO = 0x38,
  HEXO = 0x40,
  STRO = 0x48,

  ADDSP = 0x50,
  SUBSP = 0x58,
  ADDA = 0x60,
  ADDX = 0x68,
  SUBA = 0x70,
  SUBX = 0x78,
  ANDA = 0x80,
  ANDX = 0x88,
  ORA = 0x90,
  ORX = 0x98,

  CPWA = 0xA0,
  CPWX = 0xA8,
  CPBA = 0xB0,
  CPBX = 0xB8,

  LDWA = 0xC0,
  LDWX = 0xC8,
  LDBA = 0xD0,
  LDBX = 0xD8,
  STWA = 0xE0,
  STWX = 0xE8,
  STBA = 0xF0,
  STBX = 0xF8,

  INVALID,
};

enum class AddressingMode {
  NONE = 0,
  I = 1,
  D = 2,
  N = 4,
  S = 8,
  SF = 16,
  X = 32,
  SX = 64,
  SFX = 128,
  ALL = 255,
  INVALID
};

enum class InstructionType {
  Invalid,
  U_none,   //?
  R_none,   //?
  N_none,   //?
  A_ix,     //?
  AAA_noi,  //?
  AAA_stro, //? d, n, s, sf, x
  AAA_all,  //?
  AAA_i,    //?
  RAAA_all, //?
  RAAA_noi
};
struct Instruction {
  Mnemonic mnemon;
  InstructionType type;
  bool unary;
};
struct Opcode {
  Instruction instr;
  AddressingMode mode;
  bool valid;
};

enum class Register : uint8_t { A = 0, X = 1, SP = 2, PC = 3, IS = 4, OS = 5, INVALID };

constexpr std::pair<std::array<Opcode, 256>, isa::OpcodePlane> initOpcodes() {
  using M = Mnemonic;
  using T = InstructionType;
  using AM = AddressingMode;
  auto mn = std::array<Opcode, 256>();
  auto bh = isa::OpcodePlane();
  auto add_ix = [&mn](Instruction i) {
    auto base = static_cast<uint8_t>(i.mnemon);
    mn[base] = {.instr = i, .mode = AM::I, .valid = true};
    mn[base + 1] = {.instr = i, .mode = AM::X, .valid = true};
  };
  auto bh_ix = [&bh](u8 base, isa::SharedOpBehavior b) {
    bh[base] = {.behavior = b, .addr = isa::SharedAddrMode::I};
    bh[base + 1] = {.behavior = b, .addr = isa::SharedAddrMode::X};
  };
  auto bh_unary_aaa = [&bh](u8 base, isa::SharedOpBehavior b, Register r = Register ::INVALID) {
    bh[base] = {.behavior = b, .addr = isa::SharedAddrMode::Unary, .target = static_cast<u8>(r)};
    bh[base + 1] = {.behavior = b, .addr = isa::SharedAddrMode::Unary, .target = static_cast<u8>(r)};
    bh[base + 2] = {.behavior = b, .addr = isa::SharedAddrMode::Unary, .target = static_cast<u8>(r)};
    bh[base + 3] = {.behavior = b, .addr = isa::SharedAddrMode::Unary, .target = static_cast<u8>(r)};
    bh[base + 4] = {.behavior = b, .addr = isa::SharedAddrMode::Unary, .target = static_cast<u8>(r)};
    bh[base + 5] = {.behavior = b, .addr = isa::SharedAddrMode::Unary, .target = static_cast<u8>(r)};
    bh[base + 6] = {.behavior = b, .addr = isa::SharedAddrMode::Unary, .target = static_cast<u8>(r)};
    bh[base + 7] = {.behavior = b, .addr = isa::SharedAddrMode::Unary, .target = static_cast<u8>(r)};
  };
  auto bh_all_aaa = [&bh](u8 base, isa::SharedOpBehavior b, Register r = Register ::INVALID) {
    bh[base] = {.behavior = b, .addr = isa::SharedAddrMode::I, .target = static_cast<u8>(r)};
    bh[base + 1] = {.behavior = b, .addr = isa::SharedAddrMode::D, .target = static_cast<u8>(r)};
    bh[base + 2] = {.behavior = b, .addr = isa::SharedAddrMode::N, .target = static_cast<u8>(r)};
    bh[base + 3] = {.behavior = b, .addr = isa::SharedAddrMode::S, .target = static_cast<u8>(r)};
    bh[base + 4] = {.behavior = b, .addr = isa::SharedAddrMode::SF, .target = static_cast<u8>(r)};
    bh[base + 5] = {.behavior = b, .addr = isa::SharedAddrMode::X, .target = static_cast<u8>(r)};
    bh[base + 6] = {.behavior = b, .addr = isa::SharedAddrMode::SX, .target = static_cast<u8>(r)};
    bh[base + 7] = {.behavior = b, .addr = isa::SharedAddrMode::SFX, .target = static_cast<u8>(r)};
  };
  auto bh_noi_aaa = [&bh](u8 base, isa::SharedOpBehavior b, Register r = Register ::INVALID) {
    bh[base] = {.behavior = isa::SharedOpBehavior::INVALID, .addr = isa::SharedAddrMode::Unary};
    bh[base + 1] = {.behavior = b, .addr = isa::SharedAddrMode::D, .target = static_cast<u8>(r)};
    bh[base + 2] = {.behavior = b, .addr = isa::SharedAddrMode::N, .target = static_cast<u8>(r)};
    bh[base + 3] = {.behavior = b, .addr = isa::SharedAddrMode::S, .target = static_cast<u8>(r)};
    bh[base + 4] = {.behavior = b, .addr = isa::SharedAddrMode::SF, .target = static_cast<u8>(r)};
    bh[base + 5] = {.behavior = b, .addr = isa::SharedAddrMode::X, .target = static_cast<u8>(r)};
    bh[base + 6] = {.behavior = b, .addr = isa::SharedAddrMode::SX, .target = static_cast<u8>(r)};
    bh[base + 7] = {.behavior = b, .addr = isa::SharedAddrMode::SFX, .target = static_cast<u8>(r)};
  };
  auto add_all = [&mn](Instruction i) {
    auto base = static_cast<uint8_t>(i.mnemon);
    mn[base] = {.instr = i, .mode = AM::I, .valid = true};
    mn[base + 1] = {.instr = i, .mode = AM::D, .valid = true};
    mn[base + 2] = {.instr = i, .mode = AM::N, .valid = true};
    mn[base + 3] = {.instr = i, .mode = AM::S, .valid = true};
    mn[base + 4] = {.instr = i, .mode = AM::SF, .valid = true};
    mn[base + 5] = {.instr = i, .mode = AM::X, .valid = true};
    mn[base + 6] = {.instr = i, .mode = AM::SX, .valid = true};
    mn[base + 7] = {.instr = i, .mode = AM::SFX, .valid = true};
  };

  mn[(uint8_t)M::STOP] = {
      .instr = {.mnemon = M::STOP, .type = T::U_none, .unary = true}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::STOP] = {.behavior = isa::SharedOpBehavior::STOP, .addr = isa::SharedAddrMode::Unary};
  mn[(uint8_t)M::RET] = {.instr = {.mnemon = M::RET, .type = T::U_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::RET] = {.behavior = isa::SharedOpBehavior::RET, .addr = isa::SharedAddrMode::Unary};
  mn[(uint8_t)M::RETTR] = {
      .instr = {.mnemon = M::RETTR, .type = T::U_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::RETTR] = {.behavior = isa::SharedOpBehavior::SRET, .addr = isa::SharedAddrMode::Unary};
  mn[(uint8_t)M::MOVSPA] = {
      .instr = {.mnemon = M::MOVSPA, .type = T::U_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::MOVSPA] = {.behavior = isa::SharedOpBehavior::MOVSPA, .addr = isa::SharedAddrMode::Unary};
  mn[(uint8_t)M::MOVFLGA] = {
      .instr = {.mnemon = M::MOVFLGA, .type = T::U_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::MOVFLGA] = {.behavior = isa::SharedOpBehavior::MOVFLGA, .addr = isa::SharedAddrMode::Unary};
  mn[(uint8_t)M::MOVAFLG] = {
      .instr = {.mnemon = M::MOVAFLG, .type = T::U_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::MOVAFLG] = {.behavior = isa::SharedOpBehavior::MOVAFLG, .addr = isa::SharedAddrMode::Unary};

  mn[(uint8_t)M::NOTA] = {.instr = {.mnemon = M::NOTA, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::NOTA] = {.behavior = isa::SharedOpBehavior::NOT,
                          .addr = isa::SharedAddrMode::Unary,
                          .target = static_cast<u8>(Register::A)};
  mn[(uint8_t)M::NOTX] = {.instr = {.mnemon = M::NOTX, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::NOTX] = {.behavior = isa::SharedOpBehavior::NOT,
                          .addr = isa::SharedAddrMode::Unary,
                          .target = static_cast<u8>(Register::X)};

  mn[(uint8_t)M::NEGA] = {.instr = {.mnemon = M::NEGA, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::NEGA] = {.behavior = isa::SharedOpBehavior::NEG,
                          .addr = isa::SharedAddrMode::Unary,
                          .target = static_cast<u8>(Register::A)};
  mn[(uint8_t)M::NEGX] = {.instr = {.mnemon = M::NEGX, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::NEGX] = {.behavior = isa::SharedOpBehavior::NEG,
                          .addr = isa::SharedAddrMode::Unary,
                          .target = static_cast<u8>(Register::X)};

  mn[(uint8_t)M::ASLA] = {.instr = {.mnemon = M::ASLA, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::ASLA] = {.behavior = isa::SharedOpBehavior::ASL,
                          .addr = isa::SharedAddrMode::Unary,
                          .target = static_cast<u8>(Register::A)};
  mn[(uint8_t)M::ASLX] = {.instr = {.mnemon = M::ASLX, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::ASLX] = {.behavior = isa::SharedOpBehavior::ASL,
                          .addr = isa::SharedAddrMode::Unary,
                          .target = static_cast<u8>(Register::X)};

  mn[(uint8_t)M::ASRA] = {.instr = {.mnemon = M::ASRA, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::ASRA] = {.behavior = isa::SharedOpBehavior::ASR,
                          .addr = isa::SharedAddrMode::Unary,
                          .target = static_cast<u8>(Register::A)};
  mn[(uint8_t)M::ASRX] = {.instr = {.mnemon = M::ASRX, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::ASRX] = {.behavior = isa::SharedOpBehavior::ASR,
                          .addr = isa::SharedAddrMode::Unary,
                          .target = static_cast<u8>(Register::X)};

  mn[(uint8_t)M::ROLA] = {.instr = {.mnemon = M::ROLA, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::ROLA] = {.behavior = isa::SharedOpBehavior::ROL,
                          .addr = isa::SharedAddrMode::Unary,
                          .target = static_cast<u8>(Register::A)};
  mn[(uint8_t)M::ROLX] = {.instr = {.mnemon = M::ROLX, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::ROLX] = {.behavior = isa::SharedOpBehavior::ROL,
                          .addr = isa::SharedAddrMode::Unary,
                          .target = static_cast<u8>(Register::X)};

  mn[(uint8_t)M::RORA] = {.instr = {.mnemon = M::RORA, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::RORA] = {.behavior = isa::SharedOpBehavior::ROR,
                          .addr = isa::SharedAddrMode::Unary,
                          .target = static_cast<u8>(Register::A)};
  mn[(uint8_t)M::RORX] = {.instr = {.mnemon = M::RORX, .type = T::R_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::RORX] = {.behavior = isa::SharedOpBehavior::ROR,
                          .addr = isa::SharedAddrMode::Unary,
                          .target = static_cast<u8>(Register::X)};

  add_ix({.mnemon = M::BR, .type = T::A_ix, .unary = 0});
  bh_ix(static_cast<uint8_t>(M::BR), isa::SharedOpBehavior::BR);
  add_ix({.mnemon = M::BRLE, .type = T::A_ix, .unary = 0});
  bh_ix(static_cast<uint8_t>(M::BRLE), isa::SharedOpBehavior::BRLE);
  add_ix({.mnemon = M::BRLT, .type = T::A_ix, .unary = 0});
  bh_ix(static_cast<uint8_t>(M::BRLT), isa::SharedOpBehavior::BRLT);
  add_ix({.mnemon = M::BREQ, .type = T::A_ix, .unary = 0});
  bh_ix(static_cast<uint8_t>(M::BREQ), isa::SharedOpBehavior::BREQ);
  add_ix({.mnemon = M::BRNE, .type = T::A_ix, .unary = 0});
  bh_ix(static_cast<uint8_t>(M::BRNE), isa::SharedOpBehavior::BRNE);
  add_ix({.mnemon = M::BRGE, .type = T::A_ix, .unary = 0});
  bh_ix(static_cast<uint8_t>(M::BRGE), isa::SharedOpBehavior::BRGE);
  add_ix({.mnemon = M::BRGT, .type = T::A_ix, .unary = 0});
  bh_ix(static_cast<uint8_t>(M::BRGT), isa::SharedOpBehavior::BRGT);
  add_ix({.mnemon = M::BRV, .type = T::A_ix, .unary = 0});
  bh_ix(static_cast<uint8_t>(M::BRV), isa::SharedOpBehavior::BRV);
  add_ix({.mnemon = M::BRC, .type = T::A_ix, .unary = 0});
  bh_ix(static_cast<uint8_t>(M::BRC), isa::SharedOpBehavior::BRC);
  add_ix({.mnemon = M::CALL, .type = T::A_ix, .unary = 0});
  bh_ix(static_cast<uint8_t>(M::CALL), isa::SharedOpBehavior::CALL);
  // Add unary traps
  mn[(uint8_t)M::NOP0] = {.instr = {.mnemon = M::NOP0, .type = T::N_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::NOP0] = {.behavior = isa::SharedOpBehavior::TRAP_CALL, .addr = isa::SharedAddrMode::Unary};
  mn[(uint8_t)M::NOP1] = {.instr = {.mnemon = M::NOP1, .type = T::N_none, .unary = 1}, .mode = AM::NONE, .valid = true};
  bh[(uint8_t)M::NOP1] = {.behavior = isa::SharedOpBehavior::TRAP_CALL, .addr = isa::SharedAddrMode::Unary};
  // Add non-unary traps, and patch up the non-unary traps' addressing modes.
  add_all({.mnemon = M::NOP, .type = T::AAA_i, .unary = 1});
  bh_all_aaa(static_cast<uint8_t>(M::NOP), isa::SharedOpBehavior::TRAP_CALL);
  add_all({.mnemon = M::DECI, .type = T::AAA_noi, .unary = 1});
  bh_noi_aaa(static_cast<uint8_t>(M::DECI), isa::SharedOpBehavior::TRAP_CALL);
  mn[(uint8_t)M::DECI].valid = false;
  bh[(uint8_t)M::DECI] = {.behavior = isa::SharedOpBehavior::INVALID, .addr = isa::SharedAddrMode::Unary};
  mn[(uint8_t)M::DECI + 0].valid = false;
  bh[(uint8_t)M::DECI + 0] = {.behavior = isa::SharedOpBehavior::INVALID, .addr = isa::SharedAddrMode::Unary};
  mn[(uint8_t)M::DECI + 6].valid = false;
  bh[(uint8_t)M::DECI + 6] = {.behavior = isa::SharedOpBehavior::INVALID, .addr = isa::SharedAddrMode::Unary};
  mn[(uint8_t)M::DECI + 7].valid = false;
  bh[(uint8_t)M::DECI + 7] = {.behavior = isa::SharedOpBehavior::INVALID, .addr = isa::SharedAddrMode::Unary};
  add_all({.mnemon = M::DECO, .type = T::AAA_all, .unary = 1});
  bh_all_aaa(static_cast<uint8_t>(M::DECO), isa::SharedOpBehavior::TRAP_CALL);
  add_all({.mnemon = M::HEXO, .type = T::AAA_all, .unary = 1});
  bh_all_aaa(static_cast<uint8_t>(M::HEXO), isa::SharedOpBehavior::TRAP_CALL);
  add_all({.mnemon = M::STRO, .type = T::AAA_stro, .unary = 1});
  bh_all_aaa(static_cast<uint8_t>(M::STRO), isa::SharedOpBehavior::TRAP_CALL);

  add_all({.mnemon = M::ADDSP, .type = T::AAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::ADDSP), isa::SharedOpBehavior::ADDSP, Register::SP);
  add_all({.mnemon = M::SUBSP, .type = T::AAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::SUBSP), isa::SharedOpBehavior::SUBSP, Register::SP);

  add_all({.mnemon = M::ADDA, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::ADDA), isa::SharedOpBehavior::ADD, Register::A);
  add_all({.mnemon = M::ADDX, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::ADDX), isa::SharedOpBehavior::ADD, Register::X);

  add_all({.mnemon = M::SUBA, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::SUBA), isa::SharedOpBehavior::SUB, Register::A);
  add_all({.mnemon = M::SUBX, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::SUBX), isa::SharedOpBehavior::SUB, Register::X);

  add_all({.mnemon = M::ANDA, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::ANDA), isa::SharedOpBehavior::AND, Register::A);
  add_all({.mnemon = M::ANDX, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::ANDX), isa::SharedOpBehavior::AND, Register::X);

  add_all({.mnemon = M::ORA, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::ORA), isa::SharedOpBehavior::OR, Register::A);
  add_all({.mnemon = M::ORX, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::ORX), isa::SharedOpBehavior::OR, Register::X);

  add_all({.mnemon = M::CPWA, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::CPWA), isa::SharedOpBehavior::CPW, Register::A);
  add_all({.mnemon = M::CPWX, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::CPWX), isa::SharedOpBehavior::CPW, Register::X);
  add_all({.mnemon = M::CPBA, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::CPBA), isa::SharedOpBehavior::CPB, Register::A);
  add_all({.mnemon = M::CPBX, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::CPBX), isa::SharedOpBehavior::CPB, Register::X);

  add_all({.mnemon = M::LDWA, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::LDWA), isa::SharedOpBehavior::LDW, Register::A);
  add_all({.mnemon = M::LDWX, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::LDWX), isa::SharedOpBehavior::LDW, Register::X);

  add_all({.mnemon = M::LDBA, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::LDBA), isa::SharedOpBehavior::LDB, Register::A);
  add_all({.mnemon = M::LDBX, .type = T::RAAA_all, .unary = 0});
  bh_all_aaa(static_cast<uint8_t>(M::LDBX), isa::SharedOpBehavior::LDB, Register::X);

  add_all({.mnemon = M::STWA, .type = T::RAAA_noi, .unary = 0});
  bh_noi_aaa(static_cast<uint8_t>(M::STWA), isa::SharedOpBehavior::STW, Register::A);
  add_all({.mnemon = M::STWX, .type = T::RAAA_noi, .unary = 0});
  bh_noi_aaa(static_cast<uint8_t>(M::STWX), isa::SharedOpBehavior::STW, Register::X);

  add_all({.mnemon = M::STBA, .type = T::RAAA_noi, .unary = 0});
  bh_noi_aaa(static_cast<uint8_t>(M::STBA), isa::SharedOpBehavior::STB, Register::A);
  add_all({.mnemon = M::STBX, .type = T::RAAA_noi, .unary = 0});
  bh_noi_aaa(static_cast<uint8_t>(M::STBX), isa::SharedOpBehavior::STB, Register::X);

  return {mn, bh};
};

enum class CSR : uint8_t { N, Z, V, C };

// TODO: Make offsets from end of OS, not absolute addresses.
enum class MemoryVectors : uint16_t {
  UserStackPtr = 0xFFFF - 0xB,   // value==0xFB8F,
  SystemStackPtr = 0xFFFF - 0x9, // value==0xFC0F
  CharIn = 0xFFFF - 0x7,         // value==0xFC15
  CharOut = 0xFFFF - 0x5,        // value==0xFC16
  Loader = 0xFFFF - 0x3,         // value==0xFC17
  TrapHandler = 0xFFFF - 0x1,    // Value==0xFC52
};
} // namespace isa::detail::pep9

namespace isa {
struct Pep9 {
  using Mnemonic = detail::pep9::Mnemonic;
  using AddressingMode = detail::pep9::AddressingMode;
  using InstructionType = detail::pep9::InstructionType;
  using Instruction = detail::pep9::Instruction;
  using Opcode = detail::pep9::Opcode;
  using Register = detail::pep9::Register;
  using CSR = detail::pep9::CSR;
  using MemoryVectors = detail::pep9::MemoryVectors;
  static constexpr uint8_t RegisterCount = 7;
  static constexpr uint8_t CSRCount = 4;

  static std::vector<std::string> const &mnemonics();
  static Mnemonic defaultMnemonic();
  static AddressingMode defaultAddressingMode();
  static AddressingMode defaultAddressingMode(Mnemonic mnemonic);
  static uint8_t opcode(Mnemonic mnemonic);
  static uint8_t opcode(Mnemonic mnemonic, AddressingMode addr);
  static AddressingMode parseAddressingMode(const std::string &addr);
  static Mnemonic parseMnemonic(const std::string &mnemonic);
  static Register parseRegister(const std::string &mnemonic);
  static std::string string(Mnemonic mnemonic);
  static std::string string(AddressingMode addr);
  static std::string string(Register reg);
  // SCALL is a non-unary mnemonic, but a unary opcode;
  static bool isMnemonicUnary(Mnemonic mnemonic);
  static bool isMnemonicUnary(uint8_t opcode);
  static bool isOpcodeUnary(Mnemonic mnemonic);
  static bool isOpcodeUnary(uint8_t opcode);
  static bool isStore(Mnemonic mnemonic);
  static bool isStore(uint8_t opcode);
  static uint8_t operandBytes(Mnemonic mnemonic);
  static uint8_t operandBytes(uint8_t opcode);
  // CALL and traps NOP0/NOP1/NOP/DECI/DECO/HEXO/STRO
  static bool isCall(Mnemonic mnemonic);
  static bool isCall(uint8_t opcode);
  static bool isTrap(Mnemonic mnemonic);
  static bool isTrap(uint8_t opcode);

  static bool isUType(Mnemonic mnemonic);
  static bool isRType(Mnemonic mnemonic);
  static bool isAType(Mnemonic mnemonic);
  static bool isValidATypeAddressingMode(Mnemonic mnemonic, AddressingMode addr);
  static bool isAAAType(Mnemonic mnemonic);
  static bool isValidAAATypeAddressingMode(Mnemonic mnemonic, AddressingMode addr);
  static bool isRAAAType(Mnemonic mnemonic);
  static bool isValidRAAATypeAddressingMode(Mnemonic mnemonic, AddressingMode addr);
  static bool isValidAddressingMode(Mnemonic mnemonic, AddressingMode addr);
  // Operand specifier should be treated as signed iff addressing mode is in {i, s, sf, sx, sfx}
  static bool decodeOperandAsSigned(uint8_t opcode);
  // Describe the mnemonic, replacing specific registers with the placeholder R
  static std::string describeMnemonicUsingPlaceholders(Mnemonic mnemonic);
  // Return the binary instruction specifier, with placeholders for registers & addressing modes
  static std::string instructionSpecifierWithPlaceholders(Mnemonic mnemonic);

  static bool requiresAddressingMode(Mnemonic mnemonic);
  static bool canElideAddressingMode(Mnemonic mnemonic, AddressingMode addr);
  constexpr static std::array<Opcode, 256> opcodeLUT = std::get<0>(detail::pep9::initOpcodes());
  constexpr static isa::OpcodePlane opcode_plane = std::get<1>(detail::pep9::initOpcodes());
  static std::set<std::string> const &legalDirectives();
  static bool isLegalDirective(const std::string &directive);

  static std::unordered_map<std::string, Mnemonic, pepp::bts::ci_hash, pepp::bts::ci_eq> const &string_to_mnemonic();
  static std::map<Mnemonic, std::string> const &mnemonic_to_string();
  static std::unordered_map<std::string, AddressingMode, pepp::bts::ci_hash, pepp::bts::ci_eq> const &
  string_to_addressmode();
  static std::map<AddressingMode, std::string> const &addressmode_to_string();
  static std::unordered_map<std::string, Register, pepp::bts::ci_hash, pepp::bts::ci_eq> const &string_to_register();
  static std::map<Register, std::string> const &register_to_string();
};
} // namespace isa
