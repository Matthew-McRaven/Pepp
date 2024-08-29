/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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

#include "about/dependencies.hpp"
#include <catch.hpp>

TEST_CASE("About Dependencies", "[scope:help.about][kind:unit][arch:*]") {
  auto deps = about::dependencies();
  CHECK(deps.length() == 14);
  for (const auto &dep : deps) CHECK(dep.licenseText.size() != 0);
};