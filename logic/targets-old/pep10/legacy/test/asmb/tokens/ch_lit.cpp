#include "catch.hpp"

#include "asmb/tokenizer.hpp"
#include "asmdr/frontend/tokenizer.hpp"
#include "asmdr/frontend/tokens.hpp"

TEST_CASE("Character Literals", "[asmb::pep10::tokens]") {

  using tokenizer_t = asmb::pep10::tokenizer<masm::frontend::lexer_t>;
  tokenizer_t lexer;
  SECTION("Escape Codes") {
    std::vector<std::string> codes = {"'", "b", "f", "n", "r", "t", "b", "\\"};
    for (auto entry : codes) {
      auto result = masm::frontend::tokenize<tokenizer_t>(
          fmt::format("   '\\{}'   ", entry), lexer);
      CHECK(result.success);
      CHECK(result.rest.empty());
      CHECK(!result.error_message);
      CHECK(result.tokens.size() == 1);

      CHECK(result.tokens[0].first == tokenizer_t::token_type::kCharConstant);
      CHECK(result.tokens[0].second == fmt::format("\\{}", entry));
    }
  }SECTION("Hex constants") {
    std::vector<std::string> codes = {"ab", "cd", "11", "fe", "25"};
    for (auto entry : codes) {
      auto result = masm::frontend::tokenize<tokenizer_t>(
          fmt::format("   '\\x{}'   ", entry), lexer);
      CHECK(result.success);
      CHECK(result.rest.empty());
      CHECK(!result.error_message);
      CHECK(result.tokens.size() == 1);

      CHECK(result.tokens[0].first == tokenizer_t::token_type::kCharConstant);
      CHECK(result.tokens[0].second == fmt::format("\\x{}", entry));
    }
  }

  SECTION("Normal text") {
    std::vector<std::string> codes = {"a", "b", "c", "d", "e", "2"};
    for (auto entry : codes) {
      auto result = masm::frontend::tokenize<tokenizer_t>(
          fmt::format("\t\t'{}'   ", entry), lexer);
      CHECK(result.success);
      CHECK(result.rest.empty());
      CHECK(!result.error_message);
      CHECK(result.tokens.size() == 1);

      CHECK(result.tokens[0].first == tokenizer_t::token_type::kCharConstant);
      CHECK(result.tokens[0].second == fmt::format("{}", entry));
    }
  }

  SECTION("No 2 plain characters") {
    auto result = masm::frontend::tokenize<tokenizer_t>("'ab'", lexer);
    CHECK(!result.success);
    CHECK(!result.rest.empty());
    CHECK(result.error_message);
  }

  SECTION("No 2 characters, escape leading") {
    auto result = masm::frontend::tokenize<tokenizer_t>("'\\na'", lexer);
    CHECK(!result.success);
    CHECK(!result.rest.empty());
    CHECK(result.error_message);
  }

  SECTION("No 2 characters, escape trailing") {
    auto result = masm::frontend::tokenize<tokenizer_t>("'a\\n'", lexer);
    CHECK(!result.success);
    CHECK(!result.rest.empty());
    CHECK(result.error_message);
  }

  SECTION("No 2 characters, both escape") {
    auto result = masm::frontend::tokenize<tokenizer_t>("'\\n\\n'", lexer);
    CHECK(!result.success);
    CHECK(!result.rest.empty());
    CHECK(result.error_message);
  }

  SECTION("No 2 characters, hex leading") {
    auto result = masm::frontend::tokenize<tokenizer_t>("'\\xcab'", lexer);
    CHECK(!result.success);
    CHECK(!result.rest.empty());
    CHECK(result.error_message);
  }

  SECTION("No 2 characters, hex trailing") {
    auto result = masm::frontend::tokenize<tokenizer_t>("'a\\xfe'", lexer);
    CHECK(!result.success);
    CHECK(!result.rest.empty());
    CHECK(result.error_message);
  }

  SECTION("No 2 characters, both hex") {
    auto result = masm::frontend::tokenize<tokenizer_t>("'\\xca\\xfe'", lexer);
    CHECK(!result.success);
    CHECK(!result.rest.empty());
    CHECK(result.error_message);
  }

  SECTION("Need 2 hex chars") {
    auto result = masm::frontend::tokenize<tokenizer_t>("'\\xf'", lexer);
    CHECK(!result.success);
    CHECK(!result.rest.empty());
    CHECK(result.error_message);
  }
}
