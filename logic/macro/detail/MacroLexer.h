
// Generated from Macro.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"


namespace macro::detail {


class  MacroLexer : public antlr4::Lexer {
public:
  enum {
    SPACING = 1, NEWLINE = 2, IDENTIFIER = 3, AT_IDENTIFIER = 4, UNSIGNED_DECIMAL = 5
  };

  explicit MacroLexer(antlr4::CharStream *input);

  ~MacroLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

}  // namespace macro::detail
