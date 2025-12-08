#include "pep_format.hpp"
#include <QStringList>
#include <fmt/format.h>
#include "toolchain2/asmb/pep_tokens.hpp"
#include "utils/textutils.hpp"

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

QString pepp::tc::format_as_columns(const QString &col0, const QString &col1, const QString &col2,
                                    const QString &col3) {
  using namespace Qt::StringLiterals;
  return rtrimmed(u"%1%2%3%4"_s
                      .arg(col0, -indents::col0_width)
                      // col1 is always identifier-like (dot commands, macros).
                      // It must not bleed into column 2, or column 2 will later be parsed as part of column 1.
                      // Conditionally insert a space to prevent that parsing issue.
                      .arg(col1.size() >= indents::col1_width ? col1 + " " : col1, -indents::col1_width)
                      .arg(col2, -indents::col2_width)
                      .arg(col3))
      .toString();
}

QString pepp::tc::format(std::span<std::shared_ptr<lex::Token> const> tokens) {
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
  QStringList arg_list;
  QString col0 = "", col1 = "", col2 = "", col3 = "", temp = "";

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
        col1 = "." + token->to_string().toUpper();
        space_after_comma = true;
        break;
      case (int)ATT::MacroInvocation:
        state = States::ARGED1;
        col1 = "@" + token->to_string();
        break;
      case (int)CTT::Identifier:
        state = States::ARGED1;
        col1 = token->to_string().toUpper();
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
        col1 = "." + token->to_string().toUpper();
        space_after_comma = true;
        break;
      case (int)ATT::MacroInvocation:
        state = States::ARGED1;
        col1 = "@" + token->to_string();
        break;
      case (int)CTT::Identifier:
        state = States::ARGED1;
        col1 = token->to_string().toUpper();
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
        col2 = arg_list.join(space_after_comma ? ", " : ",");
        state = States::END;
        break;
      case (int)CTT::InlineComment:
        col2 = arg_list.join(space_after_comma ? ", " : ",");
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
        if (scanned_args == force_lowercase_on_arg) temp = temp.toLower();
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
        col2 = arg_list.join(space_after_comma ? ", " : ",");
        state = States::END;
        break;
      case (int)CTT::InlineComment:
        col2 = arg_list.join(space_after_comma ? ", " : ",");
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
        if (scanned_args == force_lowercase_on_arg) temp = temp.toLower();
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
