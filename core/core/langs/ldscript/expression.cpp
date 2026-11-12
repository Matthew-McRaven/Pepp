/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
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
#include "core/langs/ldscript/expression.hpp"

pepp::tc::expr::Options pepp::tc::ldscript::expression_options() {
  using Options = expr::Options;
  return {.identifiers = Options::Identifiers::Dotted,
          .integers = Options::Integers::Ld,
          .newline_whitespace = true,
          .block_comments = true,
          .character_constants = false};
}
