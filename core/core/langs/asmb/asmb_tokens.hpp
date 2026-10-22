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
#include <memory>
#include "core/compile/lex/tokens.hpp"
namespace pepp::tc::expr {
struct Parsed;
}
namespace pepp::tc::lex {

enum class AsmTokenType {
  DotCommand = static_cast<int>(CommonTokenType::_FirstUser) << 0,
  CharacterConstant = static_cast<int>(CommonTokenType::_FirstUser) << 1,
  StringConstant = static_cast<int>(CommonTokenType::_FirstUser) << 2,
  MacroPlaceholder = static_cast<int>(CommonTokenType::_FirstUser) << 3,
  ParsedExpression = static_cast<int>(CommonTokenType::_FirstUser) << 4,
};

struct DotCommand : public Identifier {
  DotCommand(support::LocationInterval loc, std::string const *v);
  static constexpr int TYPE = static_cast<int>(AsmTokenType::DotCommand);
  int type() const override;
};

struct CharacterConstant : public Token {
  CharacterConstant(support::LocationInterval loc, std::string value);
  static constexpr int TYPE = static_cast<int>(AsmTokenType::CharacterConstant);
  int type() const override;
  std::string type_name() const override;
  std::string to_string() const override;
  std::string repr() const override;

  std::string value;
};

// We are going to cheat with string constants. You should drop the quotes when making an ID out of this.
// While the lexer MUST check that escape sequences are valid, it does NOT need to convert them into bytes.
struct StringConstant : public Identifier {
  StringConstant(support::LocationInterval loc, std::string const *v);
  static constexpr int TYPE = static_cast<int>(AsmTokenType::StringConstant);
  int type() const override;
  std::string type_name() const override;
  std::string to_string() const override;
  std::string repr() const override;
};

struct MacroPlaceholder : public Identifier {
  MacroPlaceholder(support::LocationInterval loc, std::string const *v);
  static constexpr int TYPE = static_cast<int>(AsmTokenType::MacroPlaceholder);
  int type() const override;
  std::string type_name() const override;
  std::string to_string() const override;
  std::string repr() const override;
};

// An expression parsed by a nested parser (e.g., the expression parser)
struct ParsedExpression : public Token {
  ParsedExpression(support::LocationInterval loc, std::shared_ptr<const pepp::tc::expr::Parsed> operand);
  static constexpr int TYPE = static_cast<int>(AsmTokenType::ParsedExpression);
  int type() const override;
  std::string type_name() const override;
  std::string to_string() const override;

  std::shared_ptr<const pepp::tc::expr::Parsed> operand;
};

} // namespace pepp::tc::lex
