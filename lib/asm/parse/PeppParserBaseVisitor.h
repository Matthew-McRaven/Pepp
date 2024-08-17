
// Generated from PeppParser.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "PeppParserVisitor.h"


namespace parse {

/**
 * This class provides an empty implementation of PeppParserVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  PeppParserBaseVisitor : public PeppParserVisitor {
public:

  virtual std::any visitProg(PeppParser::ProgContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNonUnaryInstruction(PeppParser::NonUnaryInstructionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryInstruction(PeppParser::UnaryInstructionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDirective(PeppParser::DirectiveContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInvoke_macro(PeppParser::Invoke_macroContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSymbol(PeppParser::SymbolContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInstructionLine(PeppParser::InstructionLineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDirectiveLine(PeppParser::DirectiveLineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMacroInvokeLine(PeppParser::MacroInvokeLineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCommentLine(PeppParser::CommentLineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDeferredLine(PeppParser::DeferredLineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArgument(PeppParser::ArgumentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArgument_list(PeppParser::Argument_listContext *ctx) override {
    return visitChildren(ctx);
  }


};

}  // namespace parse
