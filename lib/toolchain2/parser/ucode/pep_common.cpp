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
#include "./pep_common.hpp"

pepp::tc::ir::MemTest::MemTest(quint16 addr, quint8 value) : address(addr), size(1) {
  this->value[0] = value;
  this->value[1] = 0;
}

pepp::tc::ir::MemTest::operator QString() const {
  if (size == 2)
    return QString("Mem[0x%1]=0x%2%3")
        .arg(QString::number(address, 16), QString::number(value[0], 16), QString::number(value[1], 16));
  else return QString("Mem[0x%1]=0x%2").arg(QString::number(address, 16), QString::number(value[0], 16));
}
pepp::tc::ir::MemTest::MemTest(quint16 addr, quint16 value) : address(addr), size(2) {
  this->value[0] = (value >> 8) & 0xFF;
  this->value[1] = value & 0xff;
}
