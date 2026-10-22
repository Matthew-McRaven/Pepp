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

#include "core/langs/asmb_pep/text_format.hpp"
#include <fmt/format.h>
#include <stdexcept>
#include <string>
#include "core/compile/ir_linear/attr_comment.hpp"
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_linear/line_macro.hpp"
#include "core/compile/ir_linear/line_symbol.hpp"
#include "core/compile/ir_value/base.hpp"
#include "core/compile/macro/macro_registry.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"
#include "core/langs/asmb_pep/codegen.hpp"
#include "core/langs/asmb_pep/ir_lines.hpp"
#include "core/langs/asmb_pep/ir_visitor.hpp"
#include "core/math/bitmanip/strings.hpp"
#include "fmt/ranges.h"

std::string pepp::tc::format_as_columns(const std::string &col0, const std::string &col1, const std::string &col2,
                                        const std::string &col3) {
  using O = FormatOptions;
  const auto formatted =
      fmt::format("{:<{}}{:<{}}{:<{}}{}", col0, O::col0_width, col1.size() >= O::col1_width ? col1 + " " : col1,
                  O::col1_width, col2, O::col2_width, col3);
  return bits::rtrimmed(formatted);
}

namespace pepp::tc {
struct SourceVisitor : public PepIRVisitor {
  explicit SourceVisitor(const pep::Dialect &dialect) : dialect(dialect) {}
  const pep::Dialect &dialect;
  std::string text;
  // Format an instruction, respecting the order of operands from the line's MnemonicDescriptor
  void instruction(const LinearIR *line, const PepMnemonic &mnemonic, const pepp::ast::IRValue *value,
                   pep::AddrMode mode);
  void visit(const EmptyLine *);
  void visit(const CommentLine *);
  void visit(const SymbolLine *);
  void visit(const MonadicInstruction *);
  void visit(const DyadicInstruction *);
  void visit(const DotAlign *);
  void visit(const DotLiteral *);
  void visit(const DotBlock *);
  void visit(const DotEquate *);
  void visit(const DotSection *);
  void visit(const DotAnnotate *);
  void visit(const DotOrg *);
  void visit(const DotConditional *);
  void visit(const InlineMacroDefinition *);
  void visit(const MacroInstantiation *);
};
} // namespace pepp::tc

void pepp::tc::SourceVisitor::visit(const EmptyLine *) { text = ""; }

void pepp::tc::SourceVisitor::visit(const CommentLine *line) {
  auto comment = ";" + line->comment.value;
  text = format_as_columns(comment, "", "", "");
}

void pepp::tc::SourceVisitor::visit(const SymbolLine *line) {
  std::string comment = "";
  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  text = format_as_columns(std::string{line->symbol.entry->name} + ":", "", "", comment);
}

void pepp::tc::SourceVisitor::instruction(const LinearIR *line, const PepMnemonic &mnemonic,
                                          const pepp::ast::IRValue *value, pep::AddrMode mode) {
  using OT = pep::MnemonicDescriptor::OperandType;
  std::string symbol = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<SymbolDeclaration>(); maybe_symbol)
    symbol = std::string{maybe_symbol->entry->name} + ":";
  std::vector<std::string> operands;
  for (const auto operand : mnemonic.mn.operands()) {
    switch (operand) {
    case OT::Value: operands.emplace_back(value->string()); break;
    case OT::AddressingMode: operands.emplace_back(dialect.name(mode)); break;
    case OT::None: break;
    }
  }
  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  text = format_as_columns(symbol, std::string{mnemonic.name}, fmt::format("{}", fmt::join(operands, ",")), comment);
}

void pepp::tc::SourceVisitor::visit(const MonadicInstruction *line) {
  instruction(line, line->mnemonic, nullptr, line->mnemonic.mn.default_addr);
}

void pepp::tc::SourceVisitor::visit(const DyadicInstruction *line) {
  instruction(line, line->mnemonic, line->argument.value.get(), line->addr_mode.addr_mode);
}

void pepp::tc::SourceVisitor::visit(const DotAlign *line) {
  std::string symbol = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<SymbolDeclaration>(); maybe_symbol)
    symbol = std::string{maybe_symbol->entry->name} + ":";
  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  text = format_as_columns(symbol, ".ALIGN", line->argument.value->string(), comment);
}

void pepp::tc::SourceVisitor::visit(const DotLiteral *line) {
  std::string symbol = "", dot = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<SymbolDeclaration>(); maybe_symbol)
    symbol = std::string{maybe_symbol->entry->name} + ":";
  using Which = DotLiteral::Which;
  switch (line->which) {
  case Which::ASCII: dot = ".ASCII"; break;
  case Which::Byte1: dot = ".BYTE"; break;
  case Which::Byte2: dot = ".WORD"; break;
  case Which::Address2: dot = ".ADDRSS"; break;
  default: throw std::invalid_argument("Invalid DotLiteral kind");
  }

  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  text = format_as_columns(symbol, dot, line->argument.value->string(), comment);
}

void pepp::tc::SourceVisitor::visit(const DotBlock *line) {
  std::string symbol = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<SymbolDeclaration>(); maybe_symbol)
    symbol = std::string{maybe_symbol->entry->name} + ":";
  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  text = format_as_columns(symbol, ".BLOCK", line->argument.value->string(), comment);
}

void pepp::tc::SourceVisitor::visit(const DotEquate *line) {
  std::string comment = "";
  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  text = format_as_columns(std::string{line->symbol.entry->name} + ":", ".EQUATE", line->argument.value->string(),
                           comment);
}

void pepp::tc::SourceVisitor::visit(const DotSection *line) {
  std::array<std::string, 2> args;
  args[0] = fmt::format("\"{}\"", line->name.value);
  args[1] = fmt::format("\"{}\"", line->flags.to_string());
  std::string comment = "";
  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  const auto joined_args = fmt::format("{}", fmt::join(args, ", "));
  text = format_as_columns("", ".SECTION", joined_args, comment);
}

void pepp::tc::SourceVisitor::visit(const DotAnnotate *line) {
  std::string dot = "", comment = "";
  using Which = DotAnnotate::Which;
  switch (line->which) {
  case Which::END: dot = ".END"; break;
  case Which::EXPORT: dot = ".EXPORT"; break;
  case Which::INPUT: dot = ".INPUT"; break;
  case Which::OUTPUT: dot = ".OUTPUT"; break;
  case Which::SCALL: dot = ".SCALL"; break;
  }

  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  text = format_as_columns("", dot, line->argument.value ? line->argument.value->string() : "", comment);
}

void pepp::tc::SourceVisitor::visit(const DotOrg *line) {
  std::string dot = "", comment = "";
  using Behavior = DotOrg::Behavior;
  switch (line->behavior) {
  case Behavior::BURN: dot = ".BURN"; break;
  case Behavior::ORG: dot = ".ORG"; break;
  }

  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  text = format_as_columns("", dot, line->argument.value->string(), comment);
}

void pepp::tc::SourceVisitor::visit(const InlineMacroDefinition *line) {
  std::string symbol = "", comment = "";

  const auto args = fmt::format("{}", fmt::join(line->arguments, ", "));
  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;

  text = fmt::format(
      "{}\n{}\n{}",                                                 // Each macho is composed of 3 parts
      format_as_columns("", ".MACRO " + line->name, args, comment), // The declaration, which includes the arg list.
      bits::rtrimmed_view(line->body),                              // The body text, which we cannot format nicely
      format_as_columns("", ".ENDM", "", "")                        // And the trailing .endm
  );
}

void pepp::tc::SourceVisitor::visit(const DotConditional *line) {
  std::string dot = "", comment = "";
  using Behavior = DotConditional::Behavior;
  switch (line->behavior) {
  case Behavior::IF: dot = ".IF"; break;
  case Behavior::ELSEIF: dot = ".ELSEIF"; break;
  case Behavior::ELSE: dot = ".ELSE"; break;
  case Behavior::ENDIF: dot = ".ENDIF"; break;
  }
  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  text = format_as_columns("", dot, line->argument.value ? line->argument.value->string() : "", comment);
}

void pepp::tc::SourceVisitor::visit(const MacroInstantiation *line) {
  std::string symbol = "", comment = "";
  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  if (auto maybe_symbol = line->typed_attribute<SymbolDeclaration>(); maybe_symbol)
    symbol = std::string{maybe_symbol->entry->name} + ":";
  const auto joined_args = fmt::format("{}", fmt::join(line->arguments, ", "));
  text = format_as_columns(symbol, line->macro->name, joined_args, comment);
}

std::string pepp::tc::format_source(const pep::Dialect &dialect, const LinearIR *line) {
  SourceVisitor r(dialect);
  accept(r, line);
  return r.text;
}

std::vector<std::string> pepp::tc::format_source(const pep::Dialect &dialect, const IRProgram &program) {
  std::vector<std::string> ret;
  ret.reserve(program.size());
  for (const auto &line : program) {
    if (hidden_from_source(*line)) continue;
    SourceVisitor r(dialect);
    accept(r, line.get());
    ret.emplace_back(r.text);
  }
  return ret;
}
