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
#include "core/langs/asmb/macros.hpp"
#include <algorithm>
#include <deque>
#include <fmt/ranges.h>
#include <utility>
#include "core/compile/ir_linear/attr_comment.hpp"
#include "core/compile/ir_linear/attr_symbol.hpp"
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_linear/line_macro.hpp"
#include "core/compile/ir_linear/line_symbol.hpp"
#include "core/compile/lex/tokens.hpp"
#include "core/compile/macro/macro_registry.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/langs/asmb/asmb_lexer.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"
#include "core/math/bitmanip/strings.hpp"

std::vector<std::string> pepp::tc::parser::split_arguments(std::span<const std::shared_ptr<lex::Token>> tokens,
                                                           const lex::AsmbLexer &lexer) {
  const auto is_comma = [](const std::shared_ptr<lex::Token> &token) {
    return token->type() == lex::Literal::TYPE && std::static_pointer_cast<lex::Literal>(token)->literal == ",";
  };
  std::vector<std::string> ret;
  if (tokens.empty()) return ret;
  // Each argument is the text from its first token to its last excluding the ,. An empty argument has no tokens.
  auto head = tokens.begin();
  while (true) {
    const auto tail = std::find_if(head, tokens.end(), is_comma);
    if (head == tail) ret.emplace_back();
    else ret.emplace_back(lexer.view({(*head)->location().lower(), (*(tail - 1))->location().upper()}));
    if (tail == tokens.end()) break;
    head = tail + 1;
  }
  return ret;
}

void pepp::tc::parser::MacroCapture::begin(std::shared_ptr<InlineMacroDefinition> definition) {
  _definition = std::move(definition);
}

std::expected<void, pepp::tc::parser::MacroCapture::Error>
pepp::tc::parser::MacroCapture::capture(lex::AsmbLexer &lexer, MacroRegistry &registry) {
  using Kind = Error::Kind;
  const auto definition = std::exchange(_definition, nullptr);
  const auto start = lexer.current_location();
  // Extent of the body's text, if it has any.
  std::optional<support::Location> first, last;
  for (int depth = 1; depth > 0;) {
    if (!lexer.input_remains())
      return std::unexpected(Error{Kind::Unterminated, {start, lexer.current_location()}, definition->name});
    const auto token = lexer.next_token();
    if (!token) continue;
    else if (token->type() == lex::DotCommand::TYPE) {
      const auto dot = bits::to_upper(token->to_string());
      if (dot == "MACRO") ++depth;
      else if (dot == "ENDM" && --depth == 0) break;
    }
    if (!first) first = token->location().lower();
    // The body excludes the newline ending its last line.
    if (token->type() != lex::Empty::TYPE) last = token->location().upper();
  }
  // Nothing may follow .ENDM on its line.
  if (lexer.input_remains())
    if (const auto newline = lexer.next_token(); newline && newline->type() != lex::Empty::TYPE)
      return std::unexpected(Error{Kind::MissingNewline, newline->location(), definition->name});

  auto macro = std::make_shared<MacroDefinition>();
  macro->name = definition->name;
  for (const auto &argument : definition->arguments)
    macro->arguments.emplace_back(MacroDefinition::Argument{.name = argument, .default_value = std::nullopt});
  if (first && last) definition->body = macro->body = lexer.view({*first, *last});
  if (!registry.insert(macro))
    return std::unexpected(Error{Kind::Redefinition, {start, last.value_or(start)}, definition->name});
  return {};
}

pepp::tc::IRProgram pepp::tc::parser::flatten_macros(const IRProgram &program, std::optional<MacroComments> comments) {
  IRProgram ret;
  // Nested expansions are pushed to the front of the queue so they can be recursively flattened.
  std::deque<std::shared_ptr<tc::LinearIR>> work_queue(program.begin(), program.end());
  // The leader only reserves the comment's column, since the comment line adds its own.
  const std::string leader(1, comments ? comments->comment_leader : ' ');
  while (!work_queue.empty()) {
    auto line = work_queue.front();
    work_queue.pop_front();
    switch (line->type()) {
    case InlineMacroDefinition::TYPE: continue;
    case MacroInstantiation::TYPE: {
      // The symbol, an opening comment, the body, and a closing comment.
      const auto macro = std::static_pointer_cast<pepp::tc::MacroInstantiation>(line);
      std::vector<std::shared_ptr<LinearIR>> expansion;
      if (const auto symbol = macro->typed_attribute<SymbolDeclaration>(); symbol)
        expansion.emplace_back(std::make_shared<SymbolLine>(SymbolDeclaration{symbol->entry}));
      if (comments) {
        const auto comment = macro->typed_attribute<Comment>();
        const auto args = fmt::format("{}", fmt::join(macro->arguments, ", "));
        auto start = comments->columns(leader, macro->macro->name, args, comment ? leader + comment->value : "");
        expansion.emplace_back(std::make_shared<CommentLine>(Comment{start.substr(1)}));
      }
      // Drop the body's final newline for nicer listing output.
      const auto &body = macro->lines;
      const bool trailing_newline = !body.empty() && body.back()->type() == EmptyLine::TYPE;
      expansion.insert(expansion.end(), body.begin(), body.end() - (trailing_newline ? 1 : 0));
      if (comments) {
        auto end = comments->columns(leader, "End " + macro->macro->name, "", "");
        expansion.emplace_back(std::make_shared<CommentLine>(Comment{end.substr(1)}));
      }
      work_queue.insert(work_queue.begin(), expansion.begin(), expansion.end());
      continue;
    }
    // Copy all non-macro lines to the output.
    default: ret.emplace_back(line);
    }
  }
  return ret;
}
