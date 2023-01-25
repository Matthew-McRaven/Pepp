#include "catch.hpp"

#include "asmb/tokenizer.hpp"
#include "asmdr/frontend/tokenizer.hpp"
#include "asmdr/frontend/tokens.hpp"

TEST_CASE("Single Character String Literals", "[asmb::pep10::tokens]") {

  using tokenizer_t = asmb::pep10::tokenizer<masm::frontend::lexer_t>;
  tokenizer_t lexer;
  SECTION("Escape Codes") {
    std::vector<std::string> codes = {"\"", "b", "f", "n", "r", "t", "b", "\\"};
    for (auto entry : codes) {
      auto result = masm::frontend::tokenize<tokenizer_t>(
          fmt::format("   \"\\{}\"   ", entry), lexer);
      CHECK(result.success);
      CHECK(result.rest.empty());
      CHECK(!result.error_message);
      CHECK(result.tokens.size() == 1);

      CHECK(result.tokens[0].first == tokenizer_t::token_type::kStrConstant);
      CHECK(result.tokens[0].second == fmt::format("\\{}", entry));
    }
  }
  SECTION("Hex constants") {
    std::vector<std::string> codes = {"ab", "cd", "11", "fe", "25"};
    for (auto entry : codes) {
      auto result = masm::frontend::tokenize<tokenizer_t>(
          fmt::format("   \"\\x{}\"   ", entry), lexer);
      CHECK(result.success);
      CHECK(result.rest.empty());
      CHECK(!result.error_message);
      CHECK(result.tokens.size() == 1);

      CHECK(result.tokens[0].first == tokenizer_t::token_type::kStrConstant);
      CHECK(result.tokens[0].second == fmt::format("\\x{}", entry));
    }
  }

  SECTION("Normal text") {
    std::vector<std::string> codes = {"a", "b", "c", "d", "e", "2"};
    for (auto entry : codes) {
      auto result = masm::frontend::tokenize<tokenizer_t>(
          fmt::format("\t\t\"{}\"   ", entry), lexer);
      CHECK(result.success);
      CHECK(result.rest.empty());
      CHECK(!result.error_message);
      CHECK(result.tokens.size() == 1);

      CHECK(result.tokens[0].first == tokenizer_t::token_type::kStrConstant);
      CHECK(result.tokens[0].second == fmt::format("{}", entry));
    }
  }
}

TEST_CASE("Multiple Character String Literals", "[masm::tokens]") {

  using tokenizer_t = asmb::pep10::tokenizer<masm::frontend::lexer_t>;
  tokenizer_t lexer;
  SECTION("Escape Codes") {
    std::vector<std::string> codes = {"\"", "b", "f", "n", "r", "t", "b", "\\"};
    for (auto entry : codes) {
      auto formatted =
          fmt::format("\"\\{0}bc\" \"a\\{0}c\" \"ab\\{0}\"", entry);
      auto result = masm::frontend::tokenize<tokenizer_t>(formatted, lexer);
      CHECK(result.success);
      CHECK(result.rest.empty());
      CHECK(!result.error_message);
      CHECK(result.tokens.size() == 3);

      CHECK(result.tokens[0].first == tokenizer_t::token_type::kStrConstant);
      CHECK(result.tokens[0].second == fmt::format("\\{}bc", entry));

      CHECK(result.tokens[1].first == tokenizer_t::token_type::kStrConstant);
      CHECK(result.tokens[1].second == fmt::format("a\\{}c", entry));

      CHECK(result.tokens[2].first == tokenizer_t::token_type::kStrConstant);
      CHECK(result.tokens[2].second == fmt::format("ab\\{}", entry));
    }
  }
  SECTION("Hex constants") {
    std::vector<std::string> codes = {"ab", "cd", "11", "fe", "25"};
    ;
    for (auto entry : codes) {
      auto formatted =
          fmt::format("\"\\x{0}bc\" \"a\\x{0}c\" \"ab\\x{0}\"", entry);
      auto result = masm::frontend::tokenize<tokenizer_t>(formatted, lexer);
      CHECK(result.success);
      CHECK(result.rest.empty());
      CHECK(!result.error_message);
      CHECK(result.tokens.size() == 3);

      CHECK(result.tokens[0].first == tokenizer_t::token_type::kStrConstant);
      CHECK(result.tokens[0].second == fmt::format("\\x{}bc", entry));

      CHECK(result.tokens[1].first == tokenizer_t::token_type::kStrConstant);
      CHECK(result.tokens[1].second == fmt::format("a\\x{}c", entry));

      CHECK(result.tokens[2].first == tokenizer_t::token_type::kStrConstant);
      CHECK(result.tokens[2].second == fmt::format("ab\\x{}", entry));
    };
  }

  SECTION("Normal text") {
    std::vector<std::string> codes = {"a", "b", "c", "d", "e", "2", " "};
    for (auto entry : codes) {
      auto formatted = fmt::format("\"{0}bc\" \"a{0}c\" \"ab{0}\"", entry);
      auto result = masm::frontend::tokenize<tokenizer_t>(formatted, lexer);
      CHECK(result.success);
      CHECK(result.rest.empty());
      CHECK(!result.error_message);
      CHECK(result.tokens.size() == 3);

      CHECK(result.tokens[0].first == tokenizer_t::token_type::kStrConstant);
      CHECK(result.tokens[0].second == fmt::format("{}bc", entry));

      CHECK(result.tokens[1].first == tokenizer_t::token_type::kStrConstant);
      CHECK(result.tokens[1].second == fmt::format("a{}c", entry));

      CHECK(result.tokens[2].first == tokenizer_t::token_type::kStrConstant);
      CHECK(result.tokens[2].second == fmt::format("ab{}", entry));
    }
  }
}
