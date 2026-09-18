/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
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
#include "../../integers.h"
namespace pepp::bts {
// Elf program header (i.e.,, segment) enumerated constants
enum class SegmentType : u32 {
  PT_NULL = 0,
  PT_LOAD = 1,
  PT_DYNAMIC = 2,
  PT_INTERP = 3,
  PT_NOTE = 4,
  PT_SHLIB = 5,
  PT_PHDR = 6,
  PT_TLS = 7,
  PT_GNU_EH_FRAME = 0x6474e550,
  PT_GNU_STACK = 0x6474e551,
  PT_GNU_RELRO = 0x6474e552,
  PT_GNU_PROPERTY = 0x6474e553,
  PT_OPENBSD_RANDOMIZE = 0x65a3dbe6,
  PT_ARM_EXIDX = 0x70000001,
  PT_RISCV_ATTRIBUTES = 0x70000003,
};

enum class SegmentFlags : u32 {
  PF_NONE = 0,
  PF_X = 1,
  PF_W = 2,
  PF_R = 4,
};
consteval void is_bitflags(SegmentFlags);
} // namespace pepp::bts