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
#include <catch.hpp>
#include <string_view>

TEST_CASE("Handle versioned symbols", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  using pepp::link::split;
  using Kind = pepp::link::VersionedName::Kind;
  SECTION("Unversioned") {
    for (const std::string_view name : {"foo", "foo@"}) {
      CAPTURE(name);
      const auto parts = split(name);
      CHECK(parts.kind == Kind::Unversioned);
      CHECK(parts.base == name);
      CHECK(parts.version.empty());
    }
  }
  SECTION("Non-default version") {
    const auto parts = split("foo@V");
    CHECK(parts.kind == Kind::NonDefault);
    CHECK(parts.base == "foo");
    CHECK(parts.version == "V");
  }
  SECTION("Default version") {
    const auto parts = split("foo@@V");
    CHECK(parts.kind == Kind::Default);
    CHECK(parts.base == "foo");
    CHECK(parts.version == "V");
  }
  SECTION("Splits at the first @") {
    const auto parts = split("foo@V@W");
    CHECK(parts.base == "foo");
    CHECK(parts.version == "V@W");
  }
  SECTION("Empty base or version") {
    const auto no_base = split("@V");
    CHECK(no_base.kind == Kind::NonDefault);
    CHECK(no_base.base.empty());
    CHECK(no_base.version == "V");
    const auto no_version = split("foo@@");
    CHECK(no_version.kind == Kind::Default);
    CHECK(no_version.base == "foo");
    CHECK(no_version.version.empty());
  }
  SECTION("Strip default versions") {
    using pepp::link::strip_default_version;
    const pepp::link::DefaultVersionSet default_versions{{"foo", "V"}};
    CHECK(strip_default_version("foo", default_versions) == "foo");
    CHECK(strip_default_version("foo@@V", {}) == "foo");
    CHECK(strip_default_version("foo@V", {}) == "foo@V");
    CHECK(strip_default_version("foo@V", default_versions) == "foo");
    CHECK(strip_default_version("foo@W", default_versions) == "foo@W");
    CHECK(strip_default_version("bar@V", default_versions) == "bar@V");
    // Only base and version identify a default, enum is ignored.
    CHECK(default_versions.contains(pepp::link::split("foo@V")));
    CHECK(default_versions.contains(pepp::link::split("foo@@V")));
  }
}
