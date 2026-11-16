/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
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
#include <string_view>
#include "core/compile/source/location.hpp"
#include "core/langs/ldscript/ir.hpp"

namespace pepp::tc {
class DiagnosticTable;
}

namespace pepp::tc::ldscript {

// Parse a linker script. Each command that fails to parse is reported to diag and left out of the Script.
Script parse(DiagnosticTable &diag, std::string_view text, support::Location origin = support::Location(0, 0));

} // namespace pepp::tc::ldscript
