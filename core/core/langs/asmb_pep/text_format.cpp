#include "core/langs/asmb_pep/text_format.hpp"
#include <fmt/format.h>
#include <stdexcept>
#include <string>
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_linear/line_macro.hpp"
#include "core/compile/ir_value/base.hpp"
#include "core/compile/macro/macro_registry.hpp"
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
  ARGED2,            // In this state, you've seen one argument, so you expect COMMA | EMPTY | COMMENT.
  ARGED3,            // You've seen a comma, so you need to see an ARG to be valid list.
  MACRO_EXPECT_NAME, // when you hit .macro, you are looking for a space and an identifier. Then you look for an
                     // arugment list (arged1)
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

namespace {
/* A helper class to group macro placeholders around a "stem" token.
 * To format macro placeholders correctly, each of the following should format like a single token when not separated by
 * spaces. e.g., `A        \m\()A        A\m        \m\()A\m\m`. There is a special case where the stem token is itself
 * a macro placeholder, which occurs when a macro placeholder is freestanding e.g, `A     \m`
 *
 * The terminology here dervies a bit from lingusitics. The stem refers to the core token around which macro
 * placeholders are "glued on". Macro placeholders before the stem are prefixes, and those after the stem are suffixes:
 * e.g., `\prefix1\prefix2\()STEM\suffix1\suffix2`.
 *
 * Either the prefix or suffix or both can be empty, but the stem token will always be non-null if input remains.
 * There can only be one stem token per group, and only macro placeholders can be affixes (i.e., prefix or suffix).
 * The most common case is a single stem token with empty affixes.
 *
 * The prefix+stem+suffix is stored in head, whil all remaining tokens are stored in rest.
 */
struct TokenGroup {
  int stem_token_type = (int)pepp::tc::lex::CommonTokenType::Invalid;
  int stem_token_index = -1;
  std::span<std::shared_ptr<pepp::tc::lex::Token> const> head = {}, rest = {};
  // Return the stem token
  pepp::tc::lex::Token const *stem() const {
    if (stem_token_index < 0 || stem_token_index >= head.size()) return nullptr;
    return &*head[stem_token_index];
  }
  // Format the "stem" token in head specially, and use default formatting for prefix and suffix.
  std::string formatted_head(std::string formatted_stem) const {
    // Create a std::span of the
    auto prefix = head.subspan(0, stem_token_index);
    auto suffix = head.subspan(stem_token_index + 1);
    auto prefix_strs = prefix | std::views::transform([](const auto &elem) { return elem->to_string(); });
    auto suffix_strs = suffix | std::views::transform([](const auto &elem) { return elem->to_string(); });
    return fmt::format("{}{}{}", fmt::join(prefix_strs, ""), formatted_stem, fmt::join(suffix_strs, ""));
  }
  // Use default formatting for all parts of the head.
  std::string formatted_head() {
    auto head_strs = head | std::views::transform([](const auto &elem) { return elem->to_string(); });
    return fmt::format("{}", fmt::join(head_strs, ""));
  }
};

// Helper to idenfitiy when two tokens are touching each other (e.g., not separated by spaces.
bool adjacent(pepp::tc::support::LocationInterval first, pepp::tc::support::LocationInterval second) {
  // By definition, an interval is adjacent to itself. Needed to establish the loop invariant for next_group.
  if (first == second) return true;
  // Otherwise intervals must describe same row.
  const bool same_row = first.upper().row == second.lower().row;
  // There is sometimes an off-by-1 where the upper.column ==lower.column
  // Rather than have complex == tests, we can just compute the distance.
  const bool adjacent_columns = second.lower().column - first.upper().column <= 0;
  return same_row && adjacent_columns;
}

// Given a span of tokens, return a TokenGroup representing the longest prefix of tokens that meets the requirements
// prefix+stem+suffix. The remaining tokens are in the "rest" field of the return value. Will return an empty/invalid
// group if input is already exhausted.
TokenGroup next_group(std::span<std::shared_ptr<pepp::tc::lex::Token> const> tokens) {
  using CTT = pepp::tc::lex::CommonTokenType;
  // comments/empty/eof/literals to "combine" with macro placeholders either to their left or their right.
  static const int left_uncombinable = (int)CTT::InlineComment | (int)CTT::Empty | (int)CTT::EoF | (int)CTT::Literal;
  // Symbol declarations allow placeholders to combine from the left but not the right.
  // e.g., `\a\()World:` is fine, but `world:\a` is not.
  static const int right_uncombinable = left_uncombinable | (int)CTT::SymbolDeclaration;
  if (tokens.empty()) return {};
  auto it = tokens.begin(), stem_token_iterator = tokens.begin(), prev = it;

  // Consume adjacent tokens until we find a non-macro-placeholder token, which is the stem.
  while (it != tokens.end() && adjacent((*prev)->location(), (*it)->location())) {
    // Avoid combining specified token types with placeholders to their left.
    if (const int type = (*it)->type(); (type & left_uncombinable) != 0) {
      // If starting with an uncombinable token, it must be consumed to ensure forward progress.
      if (prev == it) it++;
      // Current value of it becomes the first token of rest.
      goto ret;
    } else if (type != (int)pepp::tc::lex::AsmTokenType::MacroPlaceholder) {
      prev = it, stem_token_iterator = it++;
      break;
    } else prev = it++;
  }

  // Avoid combining specified tokens with placeholders to their right.
  if (((*stem_token_iterator)->type() & right_uncombinable) != 0) goto ret;

  // Continue consuming adjacent tokens after the stem, which are the suffixes. Do not consume a token which could be
  // the next group's stem.
  while (it != tokens.end() && adjacent((*prev)->location(), (*it)->location())) {
    if (const int type = (*it)->type(); type != (int)pepp::tc::lex::AsmTokenType::MacroPlaceholder) {
      // If prev is begin (occurs w/2 adjacent stem tokens), do not reset. Else head is empty, CTD ensues.
      if (prev != tokens.begin()) it = prev;
      break;
    } else prev = it++;
  }
ret:
  return TokenGroup{.stem_token_type = (*stem_token_iterator)->type(),
                    .stem_token_index = (i32)std::distance(tokens.begin(), stem_token_iterator),
                    .head = std::span(tokens.begin(), it),
                    .rest = std::span(it, tokens.end())};
}

} // namespace

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
  auto handle_argument = [&](auto &group) {
    state = States::ARGED2;
    scanned_args++;
    temp = group.stem()->to_string();
    if (scanned_args == force_lowercase_on_arg) bits::to_lower_inplace(temp);
    arg_list.emplace_back(group.formatted_head(temp));
  };
  // Combine the argument list into a comma-separated string, inserted into the second column.
  auto finalize_args = [&]() { col2 = fmt::format("{}", fmt::join(arg_list, space_after_comma ? ", " : ",")); };
  // Process tokens on (prefix+stem+suffix) group at a time.
  // next_group guarantees a non-empty head as long as the input is non-empty.
  std::span<std::shared_ptr<lex::Token> const> rest = tokens;
  while (!rest.empty()) {
    auto group = next_group(rest);
    rest = group.rest;
    if (!valid || state == States::END) break;
    switch (state) {
    case States::START:
      switch ((int)group.stem_token_type) {
      case (int)CTT::EoF: [[fallthrough]];
      case (int)CTT::Empty: valid = group.head.size() == 1, state = States::END; break;
      case (int)CTT::InlineComment:
        state = States::COMMENT;
        col0 = group.formatted_head(";" + group.stem()->to_string());
        break;
      case (int)CTT::SymbolDeclaration:
        state = States::SYMBOL;
        col0 = group.formatted_head(group.stem()->to_string() + ":");
        break;
      case (int)ATT::MacroPlaceholder:
        state = States::ARGED1;
        col1 = group.formatted_head();
        break;
      case (int)ATT::DotCommand:
        temp = "." + group.stem()->to_string();
        bits::to_upper_inplace(temp);
        col1 = group.formatted_head(temp);
        // Need special formatting for macros
        if (col1 == ".MACRO") state = States::MACRO_EXPECT_NAME;
        else state = States::ARGED1;
        space_after_comma = true;
        break;
      case (int)CTT::Literal:
        if (group.stem()->to_string() == ":" && group.head.size() > 1) {
          state = States::SYMBOL;
          col0 = group.formatted_head();
        } else valid = false;
        break;
      case (int)CTT::Identifier:
        state = States::ARGED1;
        temp = group.stem()->to_string();
        bits::to_upper_inplace(temp);
        col1 = group.formatted_head(temp);
        force_lowercase_on_arg = 2;
        break;
      default: valid = false;
      }
      break;

    case States::COMMENT:
      switch ((int)group.stem_token_type) {
      case (int)CTT::EoF: [[fallthrough]];
      case (int)CTT::Empty: state = States::END; break;
      default: valid = false;
      }
      break;

    case States::SYMBOL:
      switch ((int)group.stem_token_type) {
      case (int)ATT::DotCommand:
        state = States::ARGED1;
        temp = "." + group.stem()->to_string();
        bits::to_upper_inplace(temp);
        col1 = group.formatted_head(temp);
        space_after_comma = true;
        break;
      case (int)CTT::Identifier:
        state = States::ARGED1;
        temp = group.stem()->to_string();
        bits::to_upper_inplace(temp);
        col1 = (temp);
        force_lowercase_on_arg = 2;
        break;
      case (int)ATT::MacroPlaceholder:
        state = States::ARGED1;
        col1 = group.formatted_head();
        // Treat macro placeholder as-if an instruction.
        force_lowercase_on_arg = 2;
        break;
      default: valid = false;
      }
      break;

    // Optional argument OR some kind of EoL/comment
    case States::ARGED1:
      switch ((int)group.stem_token_type) {
      case (int)CTT::EoF: [[fallthrough]];
      case (int)CTT::Empty:
        finalize_args();
        state = States::END;
        break;
      case (int)CTT::InlineComment:
        finalize_args();
        col3 = ";" + group.stem()->to_string();
        state = States::COMMENT;
        break;
      case (int)CTT::Literal:
        if (group.stem()->to_string() == ",") state = States::ARGED2;
        else valid = false;
        break;
      case (int)CTT::Integer: [[fallthrough]];
      case (int)ATT::CharacterConstant: [[fallthrough]];
      case (int)ATT::StringConstant: [[fallthrough]];
      case (int)CTT::Identifier: handle_argument(group); break;
      case (int)ATT::MacroPlaceholder:
        state = States::ARGED2, scanned_args++;
        arg_list.push_back(group.formatted_head());
        break;
      default: valid = false;
      }
      break;

    // Last state was an argument! So we can be an EOL or the start of the next arg in a list
    case States::ARGED2:
      switch ((int)group.stem_token_type) {
      case (int)CTT::EoF: [[fallthrough]];
      case (int)CTT::Empty:
        finalize_args();
        state = States::END;
        break;
      case (int)CTT::InlineComment:
        finalize_args();
        col3 = ";" + group.stem()->to_string();
        state = States::COMMENT;
        break;
      case (int)CTT::Literal:
        if (group.stem()->to_string() == ",") state = States::ARGED3;
        else valid = false;
        break;
      default: valid = false;
      }
      break;

    // Search for an argument seperator
    case States::ARGED3:
      switch ((int)group.stem_token_type) {
      case (int)CTT::Integer: [[fallthrough]];
      case (int)ATT::CharacterConstant: [[fallthrough]];
      case (int)ATT::StringConstant: [[fallthrough]];
      case (int)CTT::Identifier: handle_argument(group); break;
      case (int)ATT::MacroPlaceholder:
        state = States::ARGED2, scanned_args++;
        arg_list.push_back(group.formatted_head());
        break;
      default: valid = false;
      }
      break;
    case States::MACRO_EXPECT_NAME:
      switch ((int)group.stem_token_type) {
      case (int)CTT::Identifier:
        state = States::ARGED1;
        col1 += " " + group.formatted_head(group.stem()->to_string());
        break;
      default: valid = false;
      }
    default: break;
    }
  }

  if (!valid) return "";
  // Reached end-of-input w/o hitting an EoF or empty token. This should still format correctly, and requires
  // finalization of args.
  else if (!arg_list.empty()) finalize_args();
  return format_as_columns(col0, col1, col2, col3);
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
  void visit(const InlineMacroDefinition *);
  void visit(const MacroInstantiation *);
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

void pepp::tc::SourceVisitor::visit(const MacroInstantiation *line) {

  std::string symbol = "", comment = "";
  if (auto maybe_comment = line->typed_attribute<Comment>(); maybe_comment) comment = ";" + maybe_comment->value;
  if (auto maybe_symbol = line->typed_attribute<SymbolDeclaration>(); maybe_symbol)
    symbol = std::string{maybe_symbol->entry->name} + ":";
  const auto joined_args = fmt::format("{}", fmt::join(line->arguments, ", "));
  text = format_as_columns(symbol, line->macro->name, joined_args, comment);
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
