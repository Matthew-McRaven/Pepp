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

#include "core/link/symbol_versions.hpp"

pepp::link::VersionedName pepp::link::split(std::string_view name) {
  using enum VersionedName::Kind;
  const auto at = name.find('@');
  if (at == std::string_view::npos || name.substr(at) == "@") return {.base = name};
  const bool is_default = name.substr(at).starts_with("@@");
  return {.base = name.substr(0, at),
          .version = name.substr(at + (is_default ? 2 : 1)),
          .kind = is_default ? Default : NonDefault};
}

std::string_view pepp::link::strip_default_version(std::string_view name, const DefaultVersionSet &default_versions) {
  using enum VersionedName::Kind;
  const auto split_name = split(name);
  // `foo@@V` is the default version of foo, as is `foo@V` once something defines `foo@@V`. GNU ld 2.45 resolves a
  // `foo@V` reference in one object to a `foo@@V` definition in another while mold 2.42 does so only for shared
  // libraries.
  if (split_name.kind == Default || (split_name.kind == NonDefault && default_versions.contains(split_name)))
    return split_name.base;
  return name;
}
