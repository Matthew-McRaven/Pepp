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
#include <functional>
#include <string_view>
#include <unordered_set>
#include "core/integers.h"

namespace pepp::link {
// A name split at its first `@`. Names without an at, or those of the form name@ without a suffix, are unversioned.
struct VersionedName {
  enum class Kind : u8 {
    Unversioned, // `foo` or `foo@`
    NonDefault,  // `foo@V`
    Default,     // `foo@@V`
  };
  std::string_view base, version;
  Kind kind = Kind::Unversioned;
};
VersionedName split(std::string_view name);

struct BaseVersionHash {
  auto operator()(const VersionedName &v) const noexcept {
    const std::hash<std::string_view> hash;
    return hash(v.base) ^ (hash(v.version) << 1);
  }
};
struct BaseVersionEqual {
  bool operator()(const VersionedName &lhs, const VersionedName &rhs) const noexcept {
    return lhs.base == rhs.base && lhs.version == rhs.version;
  }
};
// Every (base, version) that some `base@@version` definition made a default. Members compare by base and version
// only, so a `foo@V` reference finds the `foo@@V` that made V the default, as GNU ld matches them.
// A base with two default versions definitions strip to `base`, and resolution will report a multiple definition error.
using DefaultVersionSet = std::unordered_set<VersionedName, BaseVersionHash, BaseVersionEqual>;

// name without its version if that version is the base's default, else name unchanged. So `foo`, `foo@@V`, and `foo@V`
// (with V the default) all become `foo`.
std::string_view strip_default_version(std::string_view name, const DefaultVersionSet &default_versions);
} // namespace pepp::link
