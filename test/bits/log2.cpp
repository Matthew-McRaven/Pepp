/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "utils/bits/log2.hpp"

#include <catch.hpp>
#include <qtypes.h>

using T = std::tuple<std::string, quint16, quint16>;
TEST_CASE("Logarithm bit ops", "[scope:bits][kind:unit][arch:*]") {
  auto [_case, input, output] = GENERATE(table<std::string, quint16, quint16>({{"1", 1, 0},
                                                                               {"2", 2, 1},
                                                                               {"3", 3, 2},
                                                                               {"4", 4, 2},
                                                                               {"5", 5, 3},
                                                                               {"6", 6, 3},
                                                                               {"7", 7, 3},
                                                                               {"8", 8, 3},
                                                                               {"16", 16, 4}}));
  DYNAMIC_SECTION(_case) { CHECK(bits::ceil_log2(input) == output); }
}
