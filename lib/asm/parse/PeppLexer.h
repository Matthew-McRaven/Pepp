
// Generated from PeppLexer.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"


namespace parse {


class  PeppLexer : public antlr4::Lexer {
public:
  enum {
    SPACING = 1, NEWLINE = 2, STRING = 3, CHARACTER = 4, IDENTIFIER = 5, 
    DOLLAR = 6, PLACEHOLDER_MACRO = 7, DOT_IDENTIFIER = 8, AT_IDENTIFIER = 9, 
    COLON = 10, SYMBOL = 11, PLACEHOLDER_SYMBOL = 12, UNSIGNED_DECIMAL = 13, 
    SIGNED_DECIMAL = 14, HEXADECIMAL = 15, COMMENT = 16, COMMA = 17
  };

  explicit PeppLexer(antlr4::CharStream *input);

  ~PeppLexer() override;


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

}  // namespace parse
