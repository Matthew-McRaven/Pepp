#include "PeppLexerErrorListener.h"

bool parse::PeppLexerErrorListener::hadError()
{
    return _hadError;
}

void parse::PeppLexerErrorListener::syntaxError(antlr4::Recognizer *, antlr4::Token *, size_t line, size_t,
                                                const std::string &msg, std::exception_ptr) {
  _hadError = true;
  _errors[line - 1] = msg; // Lines are 1-indexed in ANTLR
}

void parse::PeppLexerErrorListener::reportAmbiguity(antlr4::Parser *, const antlr4::dfa::DFA &, size_t, size_t, bool,
                                                    const antlrcpp::BitSet &, antlr4::atn::ATNConfigSet *) {}

void parse::PeppLexerErrorListener::reportAttemptingFullContext(antlr4::Parser *, const antlr4::dfa::DFA &, size_t,
                                                                size_t, const antlrcpp::BitSet &,
                                                                antlr4::atn::ATNConfigSet *) {}

void parse::PeppLexerErrorListener::reportContextSensitivity(antlr4::Parser *, const antlr4::dfa::DFA &, size_t, size_t,
                                                             size_t, antlr4::atn::ATNConfigSet *) {}
