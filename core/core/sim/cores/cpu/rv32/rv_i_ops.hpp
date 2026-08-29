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
#pragma once
#include "core/integers.h"

// The RV32I ALU operations as pure functions for ease of testing. Uses u32 operands to avoid C++ UB on integer
// overflow. Operations which are defined by the ISA to work on signed value perform casts internally. Shift amounts are
// masked to the correct number of bits.
namespace rv32 {
constexpr u32 op_add(u32 a, u32 b) { return a + b; }
constexpr u32 op_sub(u32 a, u32 b) { return a - b; }
constexpr u32 op_sll(u32 a, u32 shamt) { return a << (shamt & 0x1F); }
constexpr u32 op_slt(u32 a, u32 b) { return static_cast<i32>(a) < static_cast<i32>(b); }
constexpr u32 op_sltu(u32 a, u32 b) { return static_cast<u32>(a < b); }
constexpr u32 op_xor(u32 a, u32 b) { return a ^ b; }
constexpr u32 op_srl(u32 a, u32 shamt) { return a >> (shamt & 0x1F); }
// C++20 onwards defines >> on a negative left operand as an arithmetic shift, which is why
constexpr u32 op_sra(u32 a, u32 shamt) { return static_cast<i32>(a) >> (shamt & 0x1F); }
constexpr u32 op_or(u32 a, u32 b) { return a | b; }
constexpr u32 op_and(u32 a, u32 b) { return a & b; }
} // namespace rv32
