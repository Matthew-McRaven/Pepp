#include "core/langs/asmb_riscv/lexer.hpp"

pepp::langs::RISCVLexer::RISCVLexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool,
                                    tc::support::SeekableData &&data)
    : AsmbLexer(identifier_pool, std::move(data), options()) {}

pepp::tc::lex::AsmbOptions pepp::langs::RISCVLexer::options() {
  return pepp::tc::lex::AsmbOptions{.allow_dot_in_ident = true,
                                    .allow_parens = true,
                                    .allow_macro_arguments = true,
                                    .allow_at_in_ident = true,
                                    .line_comment_leader = "#"};
}
