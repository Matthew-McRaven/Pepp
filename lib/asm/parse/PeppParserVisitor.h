
// Generated from PeppParser.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "PeppParser.h"


namespace parse {

/**
 * This class defines an abstract visitor for a parse tree
 * produced by PeppParser.
 */
class  PeppParserVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by PeppParser.
   */
    virtual std::any visitProg(PeppParser::ProgContext *context) = 0;

    virtual std::any visitNonUnaryInstruction(PeppParser::NonUnaryInstructionContext *context) = 0;

    virtual std::any visitUnaryInstruction(PeppParser::UnaryInstructionContext *context) = 0;

    virtual std::any visitDirective(PeppParser::DirectiveContext *context) = 0;

    virtual std::any visitInvoke_macro(PeppParser::Invoke_macroContext *context) = 0;

    virtual std::any visitSymbol(PeppParser::SymbolContext *context) = 0;

    virtual std::any visitInstructionLine(PeppParser::InstructionLineContext *context) = 0;

    virtual std::any visitDirectiveLine(PeppParser::DirectiveLineContext *context) = 0;

    virtual std::any visitMacroInvokeLine(PeppParser::MacroInvokeLineContext *context) = 0;

    virtual std::any visitCommentLine(PeppParser::CommentLineContext *context) = 0;

    virtual std::any visitDeferredLine(PeppParser::DeferredLineContext *context) = 0;

    virtual std::any visitArgument(PeppParser::ArgumentContext *context) = 0;

    virtual std::any visitArgument_list(PeppParser::Argument_listContext *context) = 0;


};

}  // namespace parse
