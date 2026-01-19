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

#include "bts/bitmanip/log2.hpp"

#include <catch.hpp>

using T = std::tuple<std::string, uint16_t, uint16_t>;
TEST_CASE("Logarithm bit ops", "[scope:bits][kind:unit][arch:*]") {
  auto [_case, input, output] = GENERATE(table<std::string, uint16_t, uint16_t>({{"1", 1, 0},
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
