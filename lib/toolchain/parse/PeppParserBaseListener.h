
// Generated from PeppParser.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "PeppParserListener.h"


namespace parse {

/**
 * This class provides an empty implementation of PeppParserListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  PeppParserBaseListener : public PeppParserListener {
public:

  virtual void enterProg(PeppParser::ProgContext * /*ctx*/) override { }
  virtual void exitProg(PeppParser::ProgContext * /*ctx*/) override { }

  virtual void enterNonUnaryInstruction(PeppParser::NonUnaryInstructionContext * /*ctx*/) override { }
  virtual void exitNonUnaryInstruction(PeppParser::NonUnaryInstructionContext * /*ctx*/) override { }

  virtual void enterUnaryInstruction(PeppParser::UnaryInstructionContext * /*ctx*/) override { }
  virtual void exitUnaryInstruction(PeppParser::UnaryInstructionContext * /*ctx*/) override { }

  virtual void enterDirective(PeppParser::DirectiveContext * /*ctx*/) override { }
  virtual void exitDirective(PeppParser::DirectiveContext * /*ctx*/) override { }

  virtual void enterInvoke_macro(PeppParser::Invoke_macroContext * /*ctx*/) override { }
  virtual void exitInvoke_macro(PeppParser::Invoke_macroContext * /*ctx*/) override { }

  virtual void enterSymbol(PeppParser::SymbolContext * /*ctx*/) override { }
  virtual void exitSymbol(PeppParser::SymbolContext * /*ctx*/) override { }

  virtual void enterInstructionLine(PeppParser::InstructionLineContext * /*ctx*/) override { }
  virtual void exitInstructionLine(PeppParser::InstructionLineContext * /*ctx*/) override { }

  virtual void enterDirectiveLine(PeppParser::DirectiveLineContext * /*ctx*/) override { }
  virtual void exitDirectiveLine(PeppParser::DirectiveLineContext * /*ctx*/) override { }

  virtual void enterMacroInvokeLine(PeppParser::MacroInvokeLineContext * /*ctx*/) override { }
  virtual void exitMacroInvokeLine(PeppParser::MacroInvokeLineContext * /*ctx*/) override { }

  virtual void enterCommentLine(PeppParser::CommentLineContext * /*ctx*/) override { }
  virtual void exitCommentLine(PeppParser::CommentLineContext * /*ctx*/) override { }

  virtual void enterDeferredLine(PeppParser::DeferredLineContext * /*ctx*/) override { }
  virtual void exitDeferredLine(PeppParser::DeferredLineContext * /*ctx*/) override { }

  virtual void enterArgument(PeppParser::ArgumentContext * /*ctx*/) override { }
  virtual void exitArgument(PeppParser::ArgumentContext * /*ctx*/) override { }

  virtual void enterArgument_list(PeppParser::Argument_listContext * /*ctx*/) override { }
  virtual void exitArgument_list(PeppParser::Argument_listContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

}  // namespace parse
