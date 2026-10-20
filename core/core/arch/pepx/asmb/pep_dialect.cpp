/*
 * /Copyright (c) 2026. Stanley Warford, Matthew McRaven
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
#include "core/arch/pepx/asmb/pep_dialect.hpp"
#include "core/ds/string_compare.hpp"

namespace {
template <typename E> std::optional<E> parse(std::span<const pep::NamedEnum<E>> names, std::string_view name) {
  for (const auto &entry : names)
    if (pepp::bts::ci_eq{}(entry.name, name)) return entry.value;
  return std::nullopt;
}

template <typename E> std::string_view name_of(std::span<const pep::NamedEnum<E>> names, E value) {
  for (const auto &entry : names)
    if (entry.value == value) return entry.name;
  return {};
}
} // namespace

std::optional<pep::AddrMode> pep::Dialect::parse_addressing_mode(std::string_view name) const {
  return parse(addressing_modes, name);
}

std::string_view pep::Dialect::name(AddrMode mode) const { return name_of(addressing_modes, mode); }

std::optional<pep::Register> pep::Dialect::parse_register(std::string_view name) const {
  return parse(registers, name);
}

std::string_view pep::Dialect::name(Register reg) const { return name_of(registers, reg); }

std::optional<pep::Directive> pep::Dialect::parse_directive(std::string_view name) const {
  const auto ret = parse<Directive>(directive_names, name);
  if (ret && (directives & mask_of(*ret))) return ret;
  return std::nullopt;
}

std::string_view pep::Dialect::name(Directive directive) const { return name_of<Directive>(directive_names, directive); }
