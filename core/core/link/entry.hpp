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
#include <optional>
#include <string>
#include "core/langs/ldscript/ir.hpp"
#include "core/link/diagnostics.hpp"
#include "core/link/link_view.hpp"
#include "core/math/bitmanip/strings.hpp"

namespace pepp::link {

// Returns the address of the code section (.text), or 0 if the output has none.
u64 code_section_address(const tc::ldscript::Sections &sections);

// Compute the entry point for an executable output according to the following algorithm:
// - the ‘-e’ entry command-line option, stored on LinkConfig::entry and passed via option
// - else the last ENTRY(symbol) command in a linker script
// - else a target-specific symbol, which is start
// If the symbol is undefined, the entry point becomes the address of the code section, which is often .text.
// Failing that, it is set to 0. Warns about the missing symbol in the first two cases.
template <ElfBits B, ElfEndian E>
u64 entry_address(Diagnostics &diagnostics, const LinkView<B, E> &link, const tc::ldscript::Script &script,
                  const std::optional<std::string> &option, const tc::ldscript::Sections &sections) {
  static constexpr auto default_name = "start";
  const auto requested = option ? option : tc::ldscript::entry_symbol(script);
  const auto symbol = requested.value_or(default_name);
  if (const auto id = link.symbols.find(symbol))
    if (const auto at = link.definitions[id.value]) return at.address;
  // option can be a number or a symbol name. ENTRY only takes names.
  if (const auto number = option ? bits::ld_integer(*option) : std::nullopt) return number->value;
  const auto ret = code_section_address(sections);
  if (requested) diagnostics.add({Severity::Warning, LinkCode::NoEntrySymbol, EntryFallback{symbol, ret}});
  return ret;
}
} // namespace pepp::link
