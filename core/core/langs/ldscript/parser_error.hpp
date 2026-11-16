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
#pragma once
#include <stdexcept>
#include <string>
#include "core/compile/source/location.hpp"

namespace pepp::tc::ldscript {

class ParserError final : public std::logic_error {
public:
  enum class NullaryError {
    Name_Expected, // Expected a symbol, section, or file name.
    Expression_Expected,
  };
  enum class UnaryError {
    Token_Expected, // Argument is the missing token (e.g., ;, =).
    Token_Invalid,
    Format_Unknown,
    Architecture_Unknown,
    Expression_Invalid, // Argument is the expression parser's message.
  };

  static const std::string to_string(NullaryError);
  static const std::string to_string(UnaryError, const std::string &arg);
  explicit ParserError(NullaryError err, support::LocationInterval);
  explicit ParserError(UnaryError err, std::string arg1, support::LocationInterval);
  const support::LocationInterval loc;
};

} // namespace pepp::tc::ldscript
