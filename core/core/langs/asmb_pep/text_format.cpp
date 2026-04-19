#include "core/langs/asmb_pep/text_format.hpp"
#include <fmt/format.h>
#include <stdexcept>
#include <string>
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_value/base.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"
#include "core/langs/asmb_pep/codegen.hpp"
#include "core/langs/asmb_pep/ir_lines.hpp"
#include "core/langs/asmb_pep/ir_visitor.hpp"
#include "core/math/bitmanip/strings.hpp"
#include "fmt/ranges.h"

namespace {
// Turns out to be a simplified version of the lexer that matches on tokens rather than characters.
enum class States {
  START,
  // You've scanned a comment, so you are expecting some kind of EoL.
  COMMENT,
  // You've scanned a symbol, so you're expecting an instrctuion, macro, or pseudo-op.

  SYMBOL,
  // Collapse instructions, dot commands, and macros into a single set of states.
  // Format will therefore accept programs the assembler would reject, like `ADDA 10,10,10`
  ARGED1, // Implements [ARG] | COMMENT | NEWLINE
  // A pair to implement (, ARG)*
  ARGED2, // In this state, you've seen one argument, so you expect COMMA | EMPTY | COMMENT.
  ARGED3, // You've seen a comma, so you need to see an ARG to be valid list.
  // Sentinel to help terminate iteration.
  END,
};
} // namespace

std::string pepp::tc::format_as_columns(const std::string &col0, const std::string &col1, const std::string &col2,
                                        const std::string &col3) {
  const auto formatted = fmt::format("{:<{}}{:<{}}{:<{}}{}", col0, FormatOptions::col0_width,
                                     col1.size() >= FormatOptions::col1_width ? col1 + " " : col1,
                                     FormatOptions::col1_width, col2, FormatOptions::col2_width, col3);
  return bits::rtrimmed(formatted);
}

std::string pepp::tc::format_source(std::span<std::shared_ptr<lex::Token> const> tokens) {
  using CTT = lex::CommonTokenType;
  using ATT = lex::AsmTokenType;

  // Instructions do not want spaces after commas, but dot commands do!
  bool valid = true, space_after_comma = false;

  // Number of arguments recognized so far.
  int scanned_args = 0;
  // Force lowercase formatting for a given arg. 1-indexed rather than 0.
  // Used to format instructions' addressing modes correctly
  int force_lowercase_on_arg = -1;
  // Accumulate arguments in a list rather directly into the corresponding column.
  std::vector<std::string> arg_list;
  std::string col0 = "", col1 = "", col2 = "", col3 = "", temp = "";

  auto state = States::START;
  for (const auto &token : tokens) {
    if (!valid || state == States::END) break;
    switch (state) {
    case States::START:
      switch ((int)token->type()) {
      case (int)CTT::EoF: [[fallthrough]];
      case (int)CTT::Empty: state = States::END; break;
      case (int)CTT::InlineComment:
        state = States::COMMENT;
        col0 = ";" + token->to_string();
        break;
      case (int)CTT::SymbolDeclaration:
        state = States::SYMBOL;
        col0 = token->to_string() + ":";
        break;
      case (int)ATT::DotCommand:
        state = States::ARGED1;
        col1 = "." + token->to_string();
        bits::to_upper_inplace(col1);
        space_after_comma = true;
        break;
      case (int)CTT::Identifier:
        state = States::ARGED1;
        col1 = token->to_string();
        bits::to_upper_inplace(col1);
        force_lowercase_on_arg = 2;
        break;
      default: valid = false;
      }
      break;

    case States::COMMENT:
      switch ((int)token->type()) {
      case (int)CTT::EoF: [[fallthrough]];
      case (int)CTT::Empty: state = States::END; break;
      default: valid = false;
      }
      break;

    case States::SYMBOL:
      switch ((int)token->type()) {
      case (int)ATT::DotCommand:
        state = States::ARGED1;
        col1 = "." + token->to_string();
        bits::to_upper_inplace(col1);
        space_after_comma = true;
        break;

      case (int)CTT::Identifier:
        state = States::ARGED1;
        col1 = token->to_string();
        bits::to_upper_inplace(col1);
        force_lowercase_on_arg = 2;
        break;
      default: valid = false;
      }
      break;

    // Optional argument OR some kind of EoL/comment
    case States::ARGED1:
      switch ((int)token->type()) {
      case (int)CTT::EoF: [[fallthrough]];
      case (int)CTT::Empty:
        col2 = fmt::format("{}", fmt::join(arg_list, space_after_comma ? ", " : ","));
        state = States::END;
        break;
      case (int)CTT::InlineComment:
        col2 = fmt::format("{}", fmt::join(arg_list, space_after_comma ? ", " : ","));
        col3 = ";" + token->to_string();
        state = States::COMMENT;
        break;
      case (int)CTT::Literal:
        if (token->to_string() == ",") state = States::ARGED2;
        else valid = false;
        break;
      case (int)CTT::Integer: [[fallthrough]];
      case (int)ATT::CharacterConstant: [[fallthrough]];
      case (int)ATT::StringConstant: [[fallthrough]];
      case (int)CTT::Identifier:
        state = States::ARGED2;
        scanned_args++;
        temp = token->to_string();
        if (scanned_args == force_lowercase_on_arg) bits::to_lower_inplace(temp);
        arg_list.emplace_back(temp);
        break;
      default: valid = false;
      }
      break;

    // Last state was an argument! So we can be an EOL or the start of the next arg in a list
    case States::ARGED2:
      switch ((int)token->type()) {
      case (int)CTT::EoF: [[fallthrough]];
      case (int)CTT::Empty:
        col2 = fmt::format("{}", fmt::join(arg_list, space_after_comma ? ", " : ","));
        state = States::END;
        break;
      case (int)CTT::InlineComment:
        col2 = fmt::format("{}", fmt::join(arg_list, space_after_comma ? ", " : ","));
        col3 = ";" + token->to_string();
        state = States::COMMENT;
        break;
      case (int)CTT::Literal:
        if (token->to_string() == ",") state = States::ARGED3;
        else valid = false;
        break;
      default: valid = false;
      }
      break;

    // Search for an argument seperator
    case States::ARGED3:
      switch ((int)token->type()) {
      case (int)CTT::Integer: [[fallthrough]];
      case (int)ATT::CharacterConstant: [[fallthrough]];
      case (int)ATT::StringConstant: [[fallthrough]];
      case (int)CTT::Identifier:
        state = States::ARGED2;
        scanned_args++;
        temp = token->to_string();
        if (scanned_args == force_lowercase_on_arg) bits::to_lower_inplace(temp);
        arg_list.emplace_back(temp);
        break;
      default: valid = false;
      }
      break;
    default: break;
    }
  }
  if (valid) return format_as_columns(col0, col1, col2, col3);
  return "";
}

namespace pepp::tc {
struct SourceVisitor : public PepIRVisitor {
  std::string text;
  void visit(const EmptyLine *);
  void visit(const CommentLine *);
  void visit(const MonadicInstruction *);
  void visit(const DyadicInstruction *);
  void visit(const DotAlign *);
  void visit(const DotLiteral *);
  void visit(const DotBlock *);
  void visit(const DotEquate *);
  void visit(const DotSection *);
  void visit(const DotAnnotate *);
  void visit(const DotOrg *);
};
} // namespace pepp::tc

void pepp::tc::SourceVisitor::visit(const EmptyLine *) { text = ""; }

void pepp::tc::SourceVisitor::visit(const CommentLine *line) {
  auto comment = ";" + line->comment.value;
  text = format_as_columns(comment, "", "", "");
}

void pepp::tc::SourceVisitor::visit(const MonadicInstruction *line) {
  std::string symbol = "", mn = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<SymbolDeclaration>(); maybe_symbol)
    symbol = std::string{maybe_symbol->entry->name} + ":";
  mn = isa::Pep10::string(line->mnemonic.instruction);
  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  text = format_as_columns(symbol, mn, "", comment);
}

void pepp::tc::SourceVisitor::visit(const DyadicInstruction *line) {
  std::vector<std::string> arg_list;
  std::string symbol = "", mn = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<SymbolDeclaration>(); maybe_symbol)
    symbol = std::string{maybe_symbol->entry->name} + ":";
  mn = isa::Pep10::string(line->mnemonic.instruction);
  arg_list.emplace_back(line->argument.value->string());
  auto addr_mode = line->addr_mode.addr_mode;
  if (!isa::Pep10::canElideAddressingMode(line->mnemonic.instruction, addr_mode))
    arg_list.emplace_back(isa::Pep10::string(addr_mode));
  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  const auto joined_args = fmt::format("{}", fmt::join(arg_list, ","));
  text = format_as_columns(symbol, mn, joined_args, comment);
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
  case Which::EXPORT: dot = ".EXPORT"; break;
  case Which::IMPORT: dot = ".IMPORT"; break;
  case Which::INPUT: dot = ".INPUT"; break;
  case Which::OUTPUT: dot = ".OUTPUT"; break;
  case Which::SCALL: dot = ".SCALL"; break;
  }

  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  text = format_as_columns("", dot, line->argument.value->string(), comment);
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

std::string pepp::tc::format_source(const LinearIR *line) {
  SourceVisitor r;
  accept(r, line);
  return r.text;
}

void format_listing(const pepp::tc::LinearIR *line,
                    const pepp::tc::IRMemoryAddressTable<pepp::tc::PeppAddress> &addresses,
                    const pepp::tc::ProgramObjectCodeResult &object_code, std::vector<std::string> &out) {
  auto source_line = pepp::tc::format_source(line);
  auto address_it = addresses.find(line);
  std::optional<u16> address =
      address_it == addresses.cend() ? std::nullopt : std::optional<u16>(address_it->second.address);

  const std::string address_str = address.has_value() ? fmt::format("{:04X}", *address) : "    ";
  auto code_it = object_code.ir_to_object_code.find(line);
  bits::span<u8> code = code_it == object_code.ir_to_object_code.end() ? bits::span<u8>{} : code_it->second;

  static constexpr int bytes_per_line = 3;
  auto obj = std::vector<char>(2 * bytes_per_line);
  auto n = std::min<size_t>(bytes_per_line, code.size());
  auto end = bits::bytesToAsciiHex(obj, code.first(n), {});
  code = code.subspan(n);

  const auto initial_line = fmt::format("{:<4} {:<6} {}", address_str, std::string_view(obj.data(), end), source_line);
  out.emplace_back(initial_line);
  while (!code.empty()) {
    n = std::min<size_t>(bytes_per_line, code.size());
    end = bits::bytesToAsciiHex(obj, code.first(n), {});
    out.emplace_back(fmt::format("     {}", std::string_view(obj.data(), end)));
    code = code.subspan(n);
  }
}

std::vector<std::string> pepp::tc::format_listing(const LinearIR *line,
                                                  const IRMemoryAddressTable<pepp::tc::PeppAddress> &addresses,
                                                  const ProgramObjectCodeResult &object_code) {
  std::vector<std::string> ret;
  ::format_listing(line, addresses, object_code, ret);
  return ret;
}

std::vector<std::string> pepp::tc::format_listing(const IRProgram &program,
                                                  const IRMemoryAddressTable<pepp::tc::PeppAddress> &addresses,
                                                  const ProgramObjectCodeResult &object_code) {
  std::vector<std::string> ret;
  for (const auto &line : program) ::format_listing(&*line, addresses, object_code, ret);
  return ret;
}
