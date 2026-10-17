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

#include "core/formats/dwarf/macro.hpp"
#include <stdexcept>

std::string pepp::bts::format_define(std::string_view name, bits::span<const std::string_view> parameters,
                                     std::string_view definition) {
  const auto has_any = [](std::string_view str, const char *chars) { return str.find_first_of(chars) != str.npos; };
  if (name.empty() || has_any(name, "( ")) throw std::logic_error("format_define: invalid macro name");
  std::string ret(name);
  ret += '(';
  for (size_t it = 0; it < parameters.size(); ++it) {
    if (parameters[it].empty() || has_any(parameters[it], ",) "))
      throw std::logic_error("format_define: invalid parameter name");
    if (it != 0) ret += ',';
    ret += parameters[it];
  }
  ret += ") ";
  ret += definition;
  return ret;
}

std::optional<pepp::bts::MacroDefineString> pepp::bts::parse_define(std::string_view string) {
  MacroDefineString ret;
  const auto split = string.find_first_of("( ");
  ret.name = string.substr(0, split);
  if (split == string.npos) return ret;
  else if (string[split] == ' ') {
    ret.definition = string.substr(split + 1);
    return ret;
  }
  const auto close = string.find(')', split);
  if (close == string.npos) return std::nullopt;
  auto &parameters = ret.parameters.emplace();
  for (auto list = string.substr(split + 1, close - split - 1); !list.empty();) {
    const auto comma = list.find(',');
    parameters.emplace_back(list.substr(0, comma));
    if (comma == list.npos) break;
    list.remove_prefix(comma + 1);
  }
  // Exactly one space separates the list from the definition, which may be empty.
  if (close + 1 < string.size()) ret.definition = string.substr(close + 2);
  return ret;
}
