#include "core/langs/asmb_pep/lexer.hpp"

pepp::tc::lex::PepLexer::PepLexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool,
                                  support::SeekableData &&data)
    : AsmbLexer(identifier_pool, std::move(data), options()) {}

pepp::tc::lex::AsmbOptions pepp::tc::lex::PepLexer::options() {
  return pepp::tc::lex::AsmbOptions{.allow_macros = true, .allow_dot_in_ident = false, .line_comment_leader = ";"};
}
