#include "pep_format.hpp"
#include <QStringList>
#include <fmt/format.h>
#include "pep_codegen.hpp"
#include "toolchain2/asmb/pep_ir_visitor.hpp"
#include "toolchain2/asmb/pep_tokens.hpp"
#include "bts/bitmanip/strings.hpp"
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
                      .arg(col0, -FormatOptions::col0_width)
                      // col1 is always identifier-like (dot commands, macros).
                      // It must not bleed into column 2, or column 2 will later be parsed as part of column 1.
                      // Conditionally insert a space to prevent that parsing issue.
                      .arg(col1.size() >= FormatOptions::col1_width ? col1 + " " : col1, -FormatOptions::col1_width)
                      .arg(col2, -FormatOptions::col2_width)
                      .arg(col3))
      .toString();
}

QString pepp::tc::format_source(std::span<std::shared_ptr<lex::Token> const> tokens) {
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

namespace pepp::tc {
struct SourceVisitor : public ir::LinearIRVisitor {
  QString text;
  void visit(const ir::EmptyLine *) override;
  void visit(const ir::CommentLine *) override;
  void visit(const ir::MonadicInstruction *) override;
  void visit(const ir::DyadicInstruction *) override;
  void visit(const ir::DotAlign *) override;
  void visit(const ir::DotLiteral *) override;
  void visit(const ir::DotBlock *) override;
  void visit(const ir::DotEquate *) override;
  void visit(const ir::DotSection *) override;
  void visit(const ir::DotAnnotate *) override;
  void visit(const ir::DotOrg *) override;
};
} // namespace pepp::tc

void pepp::tc::SourceVisitor::visit(const ir::EmptyLine *) { text = ""; }

void pepp::tc::SourceVisitor::visit(const ir::CommentLine *line) {
  auto comment = ";" + line->comment.to_string();
  text = format_as_columns(comment, "", "", "");
}

void pepp::tc::SourceVisitor::visit(const ir::MonadicInstruction *line) {
  QString symbol = "", mn = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  mn = QString::fromStdString(isa::Pep10::string(line->mnemonic.instruction));
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, mn, "", comment);
}

void pepp::tc::SourceVisitor::visit(const ir::DyadicInstruction *line) {
  QStringList arg_list;
  QString symbol = "", mn = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  mn = QString::fromStdString(isa::Pep10::string(line->mnemonic.instruction));
  arg_list.emplaceBack(line->argument.value->string());
  auto addr_mode = line->addr_mode.addr_mode;
  if (!isa::Pep10::canElideAddressingMode(line->mnemonic.instruction, addr_mode))
    arg_list.emplaceBack(QString::fromStdString(isa::Pep10::string(addr_mode)));
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, mn, arg_list.join(","), comment);
}

void pepp::tc::SourceVisitor::visit(const ir::DotAlign *line) {
  QString symbol = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, ".ALIGN", line->argument.value->string(), comment);
}

void pepp::tc::SourceVisitor::visit(const ir::DotLiteral *line) {
  QString symbol = "", dot = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  using Which = ir::DotLiteral::Which;
  switch (line->which) {
  case Which::ASCII: dot = ".ASCII"; break;
  case Which::Byte: dot = ".BYTE"; break;
  case Which::Word: dot = ".WORD"; break;
  }

  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, dot, line->argument.value->string(), comment);
}

void pepp::tc::SourceVisitor::visit(const ir::DotBlock *line) {
  QString symbol = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, ".BLOCK", line->argument.value->string(), comment);
}

void pepp::tc::SourceVisitor::visit(const ir::DotEquate *line) {
  QString comment = "";
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(line->symbol.entry->name + ":", ".EQUATE", line->argument.value->string(), comment);
}

void pepp::tc::SourceVisitor::visit(const ir::DotSection *line) {
  using namespace Qt::StringLiterals;
  QStringList args;
  args.emplaceBack(u"\"%1\""_s.arg(line->name.to_string()));
  args.emplaceBack(u"\"%1\""_s.arg(line->flags.to_string()));
  QString comment = "";
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns("", ".SECTION", args.join(", "), comment);
}

void pepp::tc::SourceVisitor::visit(const ir::DotAnnotate *line) {
  QString dot = "", comment = "";
  using Which = ir::DotAnnotate::Which;
  switch (line->which) {
  case Which::EXPORT: dot = ".EXPORT"; break;
  case Which::IMPORT: dot = ".IMPORT"; break;
  case Which::INPUT: dot = ".INPUT"; break;
  case Which::OUTPUT: dot = ".OUTPUT"; break;
  case Which::SCALL: dot = ".SCALL"; break;
  }

  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns("", dot, line->argument.value->string(), comment);
}

void pepp::tc::SourceVisitor::visit(const ir::DotOrg *line) {
  QString dot = "", comment = "";
  using Behavior = ir::DotOrg::Behavior;
  switch (line->behavior) {
  case Behavior::BURN: dot = ".BURN"; break;
  case Behavior::ORG: dot = ".ORG"; break;
  }

  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns("", dot, line->argument.value->string(), comment);
}

QString pepp::tc::format_source(const ir::LinearIR *line) {
  SourceVisitor r;
  line->accept(&r);
  return r.text;
}

void format_listing(const pepp::tc::ir::LinearIR *line, const pepp::tc::IRMemoryAddressTable &addresses,
                    const pepp::tc::ProgramObjectCodeResult &object_code, QStringList &out) {
  using namespace Qt::StringLiterals;
  auto source_line = pepp::tc::format_source(line);
  auto address_it = addresses.find(line);
  std::optional<quint16> address =
      address_it == addresses.cend() ? std::nullopt : std::optional<quint16>(address_it->second.address);
  QString address_str = address.has_value() ? u"%1"_s.arg(address.value(), 4, 16, QChar('0')) : "    ";
  auto code_it = object_code.ir_to_object_code.find(line);
  bits::span<quint8> code = code_it == object_code.ir_to_object_code.end() ? bits::span<quint8>{} : code_it->second;

  static constexpr int bytes_per_line = 3;
  auto obj = std::vector<char>(2 * bytes_per_line);
  auto n = std::min<size_t>(bytes_per_line, code.size());
  auto end = bits::bytesToAsciiHex(obj, code.first(n), {});
  code = code.subspan(n);

  auto initial_line =
      u"%1 %2 %3"_s.arg(address_str, -4).arg(QString::fromLocal8Bit(obj.data(), end), -6).arg(source_line);
  out.emplaceBack(initial_line);
  while (!code.empty()) {
    n = std::min<size_t>(bytes_per_line, code.size());
    end = bits::bytesToAsciiHex(obj, code.first(n), {});
    out.emplaceBack(u"     %1"_s.arg(QString::fromLocal8Bit(obj.data(), end)));
    code = code.subspan(n);
  }
}

QStringList pepp::tc::format_listing(const ir::LinearIR *line, const IRMemoryAddressTable &addresses,
                                     const ProgramObjectCodeResult &object_code) {
  QStringList ret;
  format_listing(line, addresses, object_code, ret);
  return ret;
}

QStringList pepp::tc::format_listing(const PepIRProgram &program, const IRMemoryAddressTable &addresses,
                                     const ProgramObjectCodeResult &object_code) {
  QStringList ret;
  for (const auto &line : program) format_listing(&*line, addresses, object_code, ret);
  return ret;
}
