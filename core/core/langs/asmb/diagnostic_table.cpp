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

#include "diagnostic_table.hpp"
#include <algorithm>
#include <string>

void pepp::tc::DiagnosticTable::add_message(support::LocationInterval i, std::string msg, Severity severity) {
  _raw.emplace(i, Diagnostic{std::move(msg), severity});
}

size_t pepp::tc::DiagnosticTable::count() const { return _raw.size(); }

size_t pepp::tc::DiagnosticTable::count(Severity minimum) const {
  return std::ranges::count_if(_raw, [minimum](const auto &entry) { return entry.second.severity >= minimum; });
}
