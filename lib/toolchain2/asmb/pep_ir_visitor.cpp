#include "./pep_ir_visitor.hpp"
#include "./pep_format.hpp"
#include "./pep_ir.hpp"

void pepp::tc::ir::SourceVisitor::visit(const EmptyLine *) { text = ""; }

void pepp::tc::ir::SourceVisitor::visit(const CommentLine *line) {
  auto comment = ";" + line->comment.to_string();
  text = format_as_columns(comment, "", "", "");
}

void pepp::tc::ir::SourceVisitor::visit(const MonadicInstruction *line) {
  QString symbol = "", mn = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  mn = isa::Pep10::string(line->mnemonic.instruction);
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, mn, "", comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DyadicInstruction *line) {
  QStringList arg_list;
  QString symbol = "", mn = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  mn = isa::Pep10::string(line->mnemonic.instruction);
  arg_list.emplaceBack(line->argument.value->string());
  auto addr_mode = line->addr_mode.addr_mode;
  if (!isa::Pep10::canElideAddressingMode(line->mnemonic.instruction, addr_mode))
    arg_list.emplaceBack(isa::Pep10::string(addr_mode));
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, mn, arg_list.join(","), comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DotAlign *line) {
  QString symbol = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, ".ALIGN", line->argument.value->string(), comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DotLiteral *line) {
  QString symbol = "", dot = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  switch (line->which) {
  case DotLiteral::Which::ASCII: dot = ".ASCII"; break;
  case DotLiteral::Which::Byte: dot = ".BYTE"; break;
  case DotLiteral::Which::Word: dot = ".WORD"; break;
  }

  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, dot, line->argument.value->string(), comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DotBlock *line) {
  QString symbol = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, ".BLOCK", line->argument.value->string(), comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DotEquate *line) {
  QString comment = "";
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(line->symbol.entry->name + ":", ".EQUATE", line->argument.value->string(), comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DotSection *line) {
  using namespace Qt::StringLiterals;
  QStringList args;
  args.emplaceBack(u"\"%1\""_s.arg(line->name.to_string()));
  args.emplaceBack(u"\"%1\""_s.arg(line->flags.to_string()));
  QString comment = "";
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns("", ".SECTION", args.join(", "), comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DotAnnotate *line) {
  QString dot = "", comment = "";
  switch (line->which) {
  case DotAnnotate::Which::EXPORT: dot = ".EXPORT"; break;
  case DotAnnotate::Which::IMPORT: dot = ".IMPORT"; break;
  case DotAnnotate::Which::INPUT: dot = ".INPUT"; break;
  case DotAnnotate::Which::OUTPUT: dot = ".OUTPUT"; break;
  case DotAnnotate::Which::SCALL: dot = ".SCALL"; break;
  }

  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns("", dot, line->argument.value->string(), comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DotOrg *line) {
  QString dot = "", comment = "";
  switch (line->behavior) {
  case DotOrg::Behavior::BURN: dot = ".BURN"; break;
  case DotOrg::Behavior::ORG: dot = ".ORG"; break;
  }

  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns("", dot, line->argument.value->string(), comment);
}

QString pepp::tc::ir::format_source(const LinearIR *line) {
  SourceVisitor r;
  line->accept(&r);
  return r.text;
}
