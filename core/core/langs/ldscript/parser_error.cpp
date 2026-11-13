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
#include "core/langs/ldscript/parser_error.hpp"
#include "core/macros.hpp"
#include "fmt/format.h"

const std::string pepp::tc::ldscript::ParserError::to_string(NullaryError err) {
  switch (err) {
  case NullaryError::Name_Expected: return "Expected a name";
  case NullaryError::Expression_Expected: return "Expected an expression";
  }
  PEPP_UNREACHABLE();
}

const std::string pepp::tc::ldscript::ParserError::to_string(UnaryError err, const std::string &arg) {
  switch (err) {
  case UnaryError::Token_Expected: return fmt::format("Expected \"{}\"", arg);
  case UnaryError::Token_Invalid: return fmt::format("Unrecognized token: {}", arg);
  case UnaryError::Format_Unknown: return fmt::format("Unknown output format \"{}\"", arg);
  case UnaryError::Architecture_Unknown: return fmt::format("Unknown architecture \"{}\"", arg);
  case UnaryError::Expression_Invalid: return fmt::format("Invalid expression: {}", arg);
  }
  PEPP_UNREACHABLE();
}

pepp::tc::ldscript::ParserError::ParserError(NullaryError err, support::LocationInterval loc)
    : std::logic_error(to_string(err)), loc(loc) {}

pepp::tc::ldscript::ParserError::ParserError(UnaryError err, std::string arg1, support::LocationInterval loc)
    : std::logic_error(to_string(err, arg1)), loc(loc) {}
