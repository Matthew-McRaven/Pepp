#pragma once
#include "core/langs/asmb/asmb_lexer.hpp"

namespace pepp::langs {

struct RISCVLexer : public pepp::tc::lex::AsmbLexer {
  RISCVLexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool, tc::support::SeekableData &&data);
  ~RISCVLexer() override = default;
  RISCVLexer(const RISCVLexer &) = delete;
  RISCVLexer &operator=(const RISCVLexer &) = delete;
  RISCVLexer(RISCVLexer &&) noexcept = default;
  RISCVLexer &operator=(RISCVLexer &&) = default;

  static pepp::tc::lex::AsmbOptions options();
};
} // namespace pepp::langs
