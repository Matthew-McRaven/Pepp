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
#include "core/arch/pep/uarch/pep.hpp"
#include "core/langs/ucode/pep_ir.hpp"
namespace pepp {
using P9Regs = pepp::tc::arch::Pep9Registers;
using P9ByteBus = pepp::tc::arch::Pep9ByteBus;
using P9WordBus = pepp::tc::arch::Pep9WordBus;
using OneByteMC9 = std::vector<typename P9ByteBus::CodeWithEnables>;
using TwoByteMC9 = std::vector<typename P9WordBus::CodeWithEnables>;
using MicrocodeChoice = std::variant<std::monostate, OneByteMC9, TwoByteMC9>;
using OneByteMC9Line = typename P9ByteBus::CodeWithEnables;
using TwoByteMC9Line = typename P9WordBus::CodeWithEnables;
using MicrocodeLineChoice = std::variant<std::monostate, OneByteMC9Line, TwoByteMC9Line>;
using P9Tests = std::vector<pepp::tc::ir::Test<pepp::tc::arch::Pep9Registers>>;
using TestChoice = std::variant<std::monostate, P9Tests>;
} // namespace pepp
