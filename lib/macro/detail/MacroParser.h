
// Generated from Macro.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"


namespace macro::detail {


class  MacroParser : public antlr4::Parser {
public:
  enum {
    SPACING = 1, NEWLINE = 2, IDENTIFIER = 3, AT_IDENTIFIER = 4, UNSIGNED_DECIMAL = 5
  };

  enum {
    RuleDecl = 0
  };

  explicit MacroParser(antlr4::TokenStream *input);

  MacroParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~MacroParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class DeclContext; 

  class  DeclContext : public antlr4::ParserRuleContext {
  public:
    DeclContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *AT_IDENTIFIER();
    antlr4::tree::TerminalNode *UNSIGNED_DECIMAL();

   
  };

  DeclContext* decl();


  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

}  // namespace macro::detail
