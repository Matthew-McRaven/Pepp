
// Generated from PeppParser.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"


namespace parse {


class  PeppParser : public antlr4::Parser {
public:
  enum {
    SPACING = 1, NEWLINE = 2, STRING = 3, CHARACTER = 4, IDENTIFIER = 5, 
    DOLLAR = 6, PLACEHOLDER_MACRO = 7, DOT_IDENTIFIER = 8, AT_IDENTIFIER = 9, 
    COLON = 10, SYMBOL = 11, PLACEHOLDER_SYMBOL = 12, UNSIGNED_DECIMAL = 13, 
    SIGNED_DECIMAL = 14, HEXADECIMAL = 15, COMMENT = 16, COMMA = 17
  };

  enum {
    RuleProg = 0, RuleInstruction = 1, RuleDirective = 2, RuleInvoke_macro = 3, 
    RuleSymbol = 4, RuleStat = 5, RuleArgument = 6, RuleArgument_list = 7
  };

  explicit PeppParser(antlr4::TokenStream *input);

  PeppParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~PeppParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  bool allow_macro_invocations = 1;
  // If true, the AST is expected to perform macro substitutions, which may require re-parsing some lines.
  // If false, macros require a pre-processor to perform macro substitution.
  bool allow_deferred_macros = 0;


  class ProgContext;
  class InstructionContext;
  class DirectiveContext;
  class Invoke_macroContext;
  class SymbolContext;
  class StatContext;
  class ArgumentContext;
  class Argument_listContext; 

  class  ProgContext : public antlr4::ParserRuleContext {
  public:
    ProgContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *EOF();
    std::vector<antlr4::tree::TerminalNode *> NEWLINE();
    antlr4::tree::TerminalNode* NEWLINE(size_t i);
    std::vector<StatContext *> stat();
    StatContext* stat(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ProgContext* prog();

  class  InstructionContext : public antlr4::ParserRuleContext {
  public:
    InstructionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    InstructionContext() = default;
    void copyFrom(InstructionContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  NonUnaryInstructionContext : public InstructionContext {
  public:
    NonUnaryInstructionContext(InstructionContext *ctx);

    std::vector<antlr4::tree::TerminalNode *> IDENTIFIER();
    antlr4::tree::TerminalNode* IDENTIFIER(size_t i);
    ArgumentContext *argument();
    antlr4::tree::TerminalNode *COMMA();
    antlr4::tree::TerminalNode *PLACEHOLDER_MACRO();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  UnaryInstructionContext : public InstructionContext {
  public:
    UnaryInstructionContext(InstructionContext *ctx);

    antlr4::tree::TerminalNode *IDENTIFIER();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  InstructionContext* instruction();

  class  DirectiveContext : public antlr4::ParserRuleContext {
  public:
    DirectiveContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DOT_IDENTIFIER();
    Argument_listContext *argument_list();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DirectiveContext* directive();

  class  Invoke_macroContext : public antlr4::ParserRuleContext {
  public:
    Invoke_macroContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *AT_IDENTIFIER();
    Argument_listContext *argument_list();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Invoke_macroContext* invoke_macro();

  class  SymbolContext : public antlr4::ParserRuleContext {
  public:
    SymbolContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SYMBOL();
    antlr4::tree::TerminalNode *PLACEHOLDER_SYMBOL();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SymbolContext* symbol();

  class  StatContext : public antlr4::ParserRuleContext {
  public:
    StatContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    StatContext() = default;
    void copyFrom(StatContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  MacroInvokeLineContext : public StatContext {
  public:
    MacroInvokeLineContext(StatContext *ctx);

    Invoke_macroContext *invoke_macro();
    SymbolContext *symbol();
    antlr4::tree::TerminalNode *COMMENT();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  CommentLineContext : public StatContext {
  public:
    CommentLineContext(StatContext *ctx);

    antlr4::tree::TerminalNode *COMMENT();
    SymbolContext *symbol();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  InstructionLineContext : public StatContext {
  public:
    InstructionLineContext(StatContext *ctx);

    InstructionContext *instruction();
    SymbolContext *symbol();
    antlr4::tree::TerminalNode *COMMENT();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  DirectiveLineContext : public StatContext {
  public:
    DirectiveLineContext(StatContext *ctx);

    DirectiveContext *directive();
    SymbolContext *symbol();
    antlr4::tree::TerminalNode *COMMENT();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  DeferredLineContext : public StatContext {
  public:
    DeferredLineContext(StatContext *ctx);

    antlr4::tree::TerminalNode *PLACEHOLDER_MACRO();
    SymbolContext *symbol();
    Argument_listContext *argument_list();
    antlr4::tree::TerminalNode *COMMENT();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  StatContext* stat();

  class  ArgumentContext : public antlr4::ParserRuleContext {
  public:
    ArgumentContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *HEXADECIMAL();
    antlr4::tree::TerminalNode *UNSIGNED_DECIMAL();
    antlr4::tree::TerminalNode *SIGNED_DECIMAL();
    antlr4::tree::TerminalNode *STRING();
    antlr4::tree::TerminalNode *CHARACTER();
    antlr4::tree::TerminalNode *IDENTIFIER();
    antlr4::tree::TerminalNode *PLACEHOLDER_MACRO();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ArgumentContext* argument();

  class  Argument_listContext : public antlr4::ParserRuleContext {
  public:
    Argument_listContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ArgumentContext *> argument();
    ArgumentContext* argument(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Argument_listContext* argument_list();


  bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

  bool progSempred(ProgContext *_localctx, size_t predicateIndex);
  bool symbolSempred(SymbolContext *_localctx, size_t predicateIndex);
  bool statSempred(StatContext *_localctx, size_t predicateIndex);
  bool argumentSempred(ArgumentContext *_localctx, size_t predicateIndex);

  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

}  // namespace parse
