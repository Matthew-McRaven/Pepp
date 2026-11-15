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
#include "core/link/symbol_inputs.hpp"
#include <algorithm>
#include <stdexcept>

std::string_view pepp::link::InputSymbols::name(InputSymbolID id) const {
  if (id.value >= _names.size()) throw std::out_of_range("InputSymbols: no symbol with this ID");
  return _names[id.value];
}

pepp::link::SymbolAttributes pepp::link::InputSymbols::attributes(InputSymbolID id) const {
  if (id.value >= _attributes.size()) throw std::out_of_range("InputSymbols: no symbol with this ID");
  return _attributes[id.value];
}

pepp::link::CommonSize pepp::link::InputSymbols::common(InputSymbolID id) const {
  const auto it = _commons.find(id);
  if (it == _commons.end()) throw std::out_of_range("InputSymbols: not a common symbol");
  return it->second;
}

std::optional<pepp::link::InputSymbolID> pepp::link::InputSymbols::find(bts::ElfFileID id, u32 index) const {
  if (!id || id.value >= _non_locals.size()) return std::nullopt;
  else if (index == 0) return InputSymbolID(0);
  // ELF puts a file's locals before its non-locals, which start at sh_info, so the non-locals are one contiguous span.
  const auto &range = _non_locals[id.value];
  if (index < range.first_global || index - range.first_global >= range.count) return std::nullopt;
  return range.first_id + (index - range.first_global);
}

pepp::bts::ElfFileID pepp::link::InputSymbols::file_of(InputSymbolID id) const {
  if (!id || id.value >= _names.size()) throw std::out_of_range("InputSymbols: no symbol with this ID");
  // Files' first IDs ascend. A file without non-locals shares its first ID with the next file, so take the last range
  // starting at or before id, which is the one holding it.
  const auto starts_after = [](InputSymbolID target, const NonLocalRange &range) { return target < range.first_id; };
  const auto it = std::upper_bound(_non_locals.begin() + 1, _non_locals.end(), id, starts_after);
  return bts::ElfFileID(static_cast<u32>(it - _non_locals.begin() - 1));
}

pepp::bts::InputElfSymbolHandle pepp::link::InputSymbols::handle_of(InputSymbolID id) const {
  const auto file = file_of(id);
  const auto &range = _non_locals[file.value];
  return {file, range.first_global + (id.value - range.first_id.value)};
}

u16 pepp::link::InputSymbols::symtab_of(bts::ElfFileID id) const {
  if (!id || id.value >= _non_locals.size()) throw std::out_of_range("InputSymbols: no file with this ID");
  return _non_locals[id.value].symtab;
}

std::pair<pepp::link::InputSymbolID, pepp::link::InputSymbolID>
pepp::link::InputSymbols::ids_of(bts::ElfFileID id) const {
  if (!id || id.value >= _non_locals.size()) throw std::out_of_range("InputSymbols: no file with this ID");
  const auto &range = _non_locals[id.value];
  return {range.first_id, range.first_id + range.count};
}