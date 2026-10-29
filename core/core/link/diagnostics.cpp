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

#include "core/link/diagnostics.hpp"
#include <algorithm>

namespace {
using namespace pepp::link;

std::string_view to_string(LinkCode code) {
  switch (code) {
  case LinkCode::NoInputs: return "no ELF input files";
  case LinkCode::Unsupported: return "option or input is not supported yet";
  }
  return "unknown error";
}
using pepp::bts::ArchiveError, pepp::bts::InputGroupError;
std::string_view to_string(InputGroupError::Code code) { return InputGroupError::to_string(code); }
std::string_view to_string(ArchiveError::Code code) { return ArchiveError::to_string(code); }
std::string_view to_string(SymbolCode code) {
  using enum SymbolCode;
  switch (code) {
  case BadStringTable: return "symbol table's string table does not exist";
  case BadFirstGlobal: return "symbol table's sh_info is out-of-bounds";
  case LocalAfterFirstGlobal: return "local symbol after sh_info";
  case GlobalBeforeFirstGlobal: return "non-local symbol before sh_info";
  case LocalOnlyType: return "non-local file or section symbol";
  case MultipleDefinition: return "multiple definition";
  case TlsMismatch: return "thread-local and non-thread-local symbols share a name";
  case Undefined: return "undefined symbol";
  }
  return "unknown error";
}

} // namespace

std::string_view pepp::link::to_string(const DiagnosticCode &code) {
  return std::visit([](auto value) { return ::to_string(value); }, code);
}

std::size_t pepp::link::Diagnostics::count(Severity minimum) const {
  return std::ranges::count_if(_entries, [minimum](const Diagnostic &entry) { return entry.severity >= minimum; });
}
