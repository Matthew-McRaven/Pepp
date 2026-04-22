#pragma once
#include <stack>
#include "core/langs/asmb/asmb_lexer.hpp"
namespace pepp::tc::lex {

struct PepLexer : public AsmbLexer {
  PepLexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool, support::SeekableData &&data);
  ~PepLexer() override = default;
  PepLexer(const PepLexer &) = delete;
  PepLexer &operator=(const PepLexer &) = delete;
  PepLexer(PepLexer &&) noexcept = default;
  PepLexer &operator=(PepLexer &&) = default;
  static pepp::tc::lex::AsmbOptions options();
};

} // namespace pepp::tc::lex
