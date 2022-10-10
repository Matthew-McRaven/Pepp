#include "catch.hpp"

#include "asmb/tokenizer.hpp"
#include "asmdr/frontend/tokenizer.hpp"
#include "asmdr/frontend/tokens.hpp"

TEST_CASE("Dot Directives", "[asmb::pep10::tokens]") {

  using tokenizer_t = asmb::pep10::tokenizer<masm::frontend::lexer_t>;
  tokenizer_t lexer;
  SECTION("First 4 dot commands") {
    auto result = masm::frontend::tokenize<tokenizer_t>(
        ".ADDRSS .ASCII .ALIGN .BLOCK", lexer);
    CHECK(result.success);
    CHECK(result.rest.empty());
    CHECK(!result.error_message);
    CHECK(result.tokens.size() == 4);

    CHECK(result.tokens[0].first == tokenizer_t::token_type::kDotDirective);
    CHECK(result.tokens[0].second == "ADDRSS");

    CHECK(result.tokens[1].first == tokenizer_t::token_type::kDotDirective);
    CHECK(result.tokens[1].second == "ASCII");

    CHECK(result.tokens[2].first == tokenizer_t::token_type::kDotDirective);
    CHECK(result.tokens[2].second == "ALIGN");

    CHECK(result.tokens[3].first == tokenizer_t::token_type::kDotDirective);
    CHECK(result.tokens[3].second == "BLOCK");
  }

  SECTION("Second 4 dot commands") {
    auto result = masm::frontend::tokenize<tokenizer_t>(
        ".burn .bYtE .ENd .Equate", lexer);
    CHECK(result.success);
    CHECK(result.rest.empty());
    CHECK(!result.error_message);
    CHECK(result.tokens.size() == 4);

    CHECK(result.tokens[0].first == tokenizer_t::token_type::kDotDirective);
    CHECK(result.tokens[0].second == "burn");

    CHECK(result.tokens[1].first == tokenizer_t::token_type::kDotDirective);
    CHECK(result.tokens[1].second == "bYtE");

    CHECK(result.tokens[2].first == tokenizer_t::token_type::kDotDirective);
    CHECK(result.tokens[2].second == "ENd");

    CHECK(result.tokens[3].first == tokenizer_t::token_type::kDotDirective);
    CHECK(result.tokens[3].second == "Equate");
  }

  SECTION("Third 4 dot commands") {
    auto result = masm::frontend::tokenize<tokenizer_t>(
        ".EXPORT .SYCALL .USYCALL .WORD", lexer);
    CHECK(result.success);
    CHECK(result.rest.empty());
    CHECK(!result.error_message);
    CHECK(result.tokens.size() == 4);

    CHECK(result.tokens[0].first == tokenizer_t::token_type::kDotDirective);
    CHECK(result.tokens[0].second == "EXPORT");

    CHECK(result.tokens[1].first == tokenizer_t::token_type::kDotDirective);
    CHECK(result.tokens[1].second == "SYCALL");

    CHECK(result.tokens[2].first == tokenizer_t::token_type::kDotDirective);
    CHECK(result.tokens[2].second == "USYCALL");

    CHECK(result.tokens[3].first == tokenizer_t::token_type::kDotDirective);
    CHECK(result.tokens[3].second == "WORD");
  }
}
