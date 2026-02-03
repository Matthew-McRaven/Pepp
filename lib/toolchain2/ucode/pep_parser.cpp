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
#include <fmt/format.h>
#include "./pep_ir.hpp"

pepp::tc::ir::MemTest::MemTest(u16 addr, u8 value) : address(addr), size(1) {
  this->value[0] = value;
  this->value[1] = 0;
}

pepp::tc::ir::MemTest::operator std::string() const {
  if (size == 2) return fmt::format("Mem[0x{:04X}]=0x{:X}{:X}", address, value[0], value[1]);
  else return fmt::format("Mem[0x{:04X}]=0x{:X}", address, value[0]);
}
pepp::tc::ir::MemTest::MemTest(u16 addr, u16 value) : address(addr), size(2) {
  this->value[0] = (value >> 8) & 0xFF;
  this->value[1] = value & 0xff;
}
