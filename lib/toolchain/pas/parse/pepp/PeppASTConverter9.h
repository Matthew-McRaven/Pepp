#pragma once

// Must be before Qt stuff, or compile errors.
#include "toolchain/parse/PeppParserBaseVisitor.h"
// So a newline is necessary.
#include <QSharedPointer>
#include <optional>

namespace pas::ast {
class Node;
}
namespace pas::ast::value {
class Base;
}
namespace symbol {
class Table;
}

namespace parse {
class PeppASTConverter9 : public PeppParserBaseVisitor {
  struct LineInfo {
    std::optional<std::string> symbol = std::nullopt;
    std::optional<std::string> identifier = std::nullopt;
    std::vector<QSharedPointer<pas::ast::value::Base>> arguments = {};
    std::optional<std::string> addr_mode = std::nullopt;
  };
  struct BlockInfo {
    QSharedPointer<symbol::Table> symTab = nullptr;
    QSharedPointer<pas::ast::Node> parent = nullptr;
  } _blockInfo;

  LineInfo _lineInfo = {};

public:
  PeppASTConverter9(QSharedPointer<pas::ast::Node> parent = nullptr);

  // PeppParserVisitor interface
  std::any visitProg(PeppParser::ProgContext *context) override;
  std::any visitNonUnaryInstruction(PeppParser::NonUnaryInstructionContext *context) override;
  std::any visitUnaryInstruction(PeppParser::UnaryInstructionContext *context) override;
  std::any visitDirective(PeppParser::DirectiveContext *context) override;
  std::any visitInvoke_macro(PeppParser::Invoke_macroContext *context) override;
  std::any visitSymbol(PeppParser::SymbolContext *context) override;
  std::any visitInstructionLine(PeppParser::InstructionLineContext *context) override;
  std::any visitDirectiveLine(PeppParser::DirectiveLineContext *context) override;
  std::any visitMacroInvokeLine(PeppParser::MacroInvokeLineContext *context) override;
  std::any visitCommentLine(PeppParser::CommentLineContext *context) override;
  std::any visitArgument(PeppParser::ArgumentContext *context) override;
  std::any visitArgument_list(PeppParser::Argument_listContext *context) override;

private:
  void addrss(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context);
  void align(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context);
  void ascii(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context);
  void block(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context);
  void burn(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context);
  void byte(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context);
  void end(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context);
  void equate(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context);
  void word(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context);
};

} // namespace parse
