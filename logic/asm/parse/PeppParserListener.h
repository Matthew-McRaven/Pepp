
// Generated from PeppParser.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "PeppParser.h"


namespace parse {

/**
 * This interface defines an abstract listener for a parse tree produced by PeppParser.
 */
class  PeppParserListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterProg(PeppParser::ProgContext *ctx) = 0;
  virtual void exitProg(PeppParser::ProgContext *ctx) = 0;

  virtual void enterNonUnaryInstruction(PeppParser::NonUnaryInstructionContext *ctx) = 0;
  virtual void exitNonUnaryInstruction(PeppParser::NonUnaryInstructionContext *ctx) = 0;

  virtual void enterUnaryInstruction(PeppParser::UnaryInstructionContext *ctx) = 0;
  virtual void exitUnaryInstruction(PeppParser::UnaryInstructionContext *ctx) = 0;

  virtual void enterDirective(PeppParser::DirectiveContext *ctx) = 0;
  virtual void exitDirective(PeppParser::DirectiveContext *ctx) = 0;

  virtual void enterInvoke_macro(PeppParser::Invoke_macroContext *ctx) = 0;
  virtual void exitInvoke_macro(PeppParser::Invoke_macroContext *ctx) = 0;

  virtual void enterSymbol(PeppParser::SymbolContext *ctx) = 0;
  virtual void exitSymbol(PeppParser::SymbolContext *ctx) = 0;

  virtual void enterInstructionLine(PeppParser::InstructionLineContext *ctx) = 0;
  virtual void exitInstructionLine(PeppParser::InstructionLineContext *ctx) = 0;

  virtual void enterDirectiveLine(PeppParser::DirectiveLineContext *ctx) = 0;
  virtual void exitDirectiveLine(PeppParser::DirectiveLineContext *ctx) = 0;

  virtual void enterMacroInvokeLine(PeppParser::MacroInvokeLineContext *ctx) = 0;
  virtual void exitMacroInvokeLine(PeppParser::MacroInvokeLineContext *ctx) = 0;

  virtual void enterCommentLine(PeppParser::CommentLineContext *ctx) = 0;
  virtual void exitCommentLine(PeppParser::CommentLineContext *ctx) = 0;

  virtual void enterDeferredLine(PeppParser::DeferredLineContext *ctx) = 0;
  virtual void exitDeferredLine(PeppParser::DeferredLineContext *ctx) = 0;

  virtual void enterArgument(PeppParser::ArgumentContext *ctx) = 0;
  virtual void exitArgument(PeppParser::ArgumentContext *ctx) = 0;

  virtual void enterArgument_list(PeppParser::Argument_listContext *ctx) = 0;
  virtual void exitArgument_list(PeppParser::Argument_listContext *ctx) = 0;


};

}  // namespace parse
