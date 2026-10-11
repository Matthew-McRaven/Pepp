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
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include "core/compile/source/location.hpp"
#include "core/langs/asmb/ir_program.hpp"

namespace pepp::tc {
class MacroRegistry;
struct InlineMacroDefinition;
namespace lex {
struct Token;
struct AsmbLexer;
} // namespace lex
} // namespace pepp::tc

namespace pepp::tc::parser {

// The source text of each comma-separated argument in tokens, e.g. the arguments of `.MACRO name a, b` or `name 1, 2`.
std::vector<std::string> split_arguments(std::span<const std::shared_ptr<lex::Token>> tokens,
                                         const lex::AsmbLexer &lexer);

// Reads the body of a .MACRO definition directly from the lexer, then registers the macro.
class MacroCapture {
public:
  struct Error {
    enum class Kind { MissingNewline, Unterminated, Redefinition } kind;
    support::LocationInterval location;
    std::string macro;
  };
  // A .MACRO line was parsed, and its body follows.
  void begin(std::shared_ptr<InlineMacroDefinition> definition);
  bool capturing() const { return _definition != nullptr; }
  // Read up to the .ENDM matching the .MACRO (a nested definition is part of the body) and the newline after it. The
  // definition's body is then set, and the macro is inserted into registry.
  std::expected<void, Error> capture(lex::AsmbLexer &lexer, MacroRegistry &registry);

private:
  std::shared_ptr<InlineMacroDefinition> _definition;
};

// Formats a listing's four columns, for comments around each expanded macro.
struct MacroComments {
  std::function<std::string(const std::string &, const std::string &, const std::string &, const std::string &)> columns;
  char comment_leader;
};
// Replace each macro instantiation with its lines, and drop macro definitions. A symbol on an instantiation moves to the
// first line of its body which allows_symbol, or to a new `.BLOCK 0` if a line with object code comes first.
IRProgram flatten_macros(const IRProgram &program, const std::function<bool(const LinearIR &)> &allows_symbol,
                         std::optional<MacroComments> comments = std::nullopt);

} // namespace pepp::tc::parser
