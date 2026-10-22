/*
 * Copyright (c) 2025-2026 J. Stanley Warford, Matthew McRaven
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
#include "core/compile/lex/lexer.hpp"
#include "core/compile/lex/tokens.hpp"
namespace pepp::tc::lex {

enum class MicrocodeTokenType {
  LineNumber = static_cast<int>(CommonTokenType::_FirstUser) << 0,
  UnitPre = static_cast<int>(CommonTokenType::_FirstUser) << 1,
  UnitPost = static_cast<int>(CommonTokenType::_FirstUser) << 2,
};

struct LineNumber : public Token {
  LineNumber(support::LocationInterval loc, int lineNo);
  static constexpr int TYPE = static_cast<int>(MicrocodeTokenType::LineNumber);
  int type() const override;
  std::string type_name() const override;
  std::string to_string() const override;
  std::string repr() const override;

  int line = 0;
};
struct UnitPre : public Token {
  UnitPre(support::LocationInterval loc);
  static constexpr int TYPE = static_cast<int>(MicrocodeTokenType::UnitPre);
  int type() const override;
  std::string type_name() const override;
  std::string to_string() const override;
  std::string repr() const override;
};
struct UnitPost : public Token {
  UnitPost(support::LocationInterval loc);
  static constexpr int TYPE = static_cast<int>(MicrocodeTokenType::UnitPost);
  int type() const override;
  std::string type_name() const override;
  std::string to_string() const override;
  std::string repr() const override;
};
} // namespace pepp::tc::lex
