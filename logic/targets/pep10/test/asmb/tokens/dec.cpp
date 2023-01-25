#include "catch.hpp"

#include "asmb/tokenizer.hpp"
#include "asmdr/frontend/tokenizer.hpp"
#include "asmdr/frontend/tokens.hpp"

TEST_CASE("Decimal Constants", "[asmb::pep10::tokens]") {

  using tokenizer_t = asmb::pep10::tokenizer<masm::frontend::lexer_t>;
  tokenizer_t lexer;
  SECTION("Positive, no leading +") {
    auto result = masm::frontend::tokenize<tokenizer_t>(
        "1 22 333 4444 55555 65537", lexer);
    CHECK(result.success);
    CHECK(result.rest.empty());
    CHECK(!result.error_message);
    CHECK(result.tokens.size() == 6);

    CHECK(result.tokens[0].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[0].second == "1");

    CHECK(result.tokens[1].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[1].second == "22");

    CHECK(result.tokens[2].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[2].second == "333");

    CHECK(result.tokens[3].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[3].second == "4444");

    CHECK(result.tokens[4].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[4].second == "55555");

    CHECK(result.tokens[5].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[5].second == "65537");
  }

  SECTION("Positive, leading +") {
    auto result = masm::frontend::tokenize<tokenizer_t>(
        "+1 +22 +333 +4444 +55555 +65537", lexer);
    CHECK(result.success);
    CHECK(result.rest.empty());
    CHECK(!result.error_message);
    CHECK(result.tokens.size() == 6);

    CHECK(result.tokens[0].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[0].second == "+1");

    CHECK(result.tokens[1].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[1].second == "+22");

    CHECK(result.tokens[2].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[2].second == "+333");

    CHECK(result.tokens[3].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[3].second == "+4444");

    CHECK(result.tokens[4].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[4].second == "+55555");

    CHECK(result.tokens[5].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[5].second == "+65537");
  }

  SECTION("Negative") {
    auto result = masm::frontend::tokenize<tokenizer_t>(
        "-1 -22 -333 -4444 -55555 -65537", lexer);
    CHECK(result.success);
    CHECK(result.rest.empty());
    CHECK(!result.error_message);
    CHECK(result.tokens.size() == 6);

    CHECK(result.tokens[0].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[0].second == "-1");

    CHECK(result.tokens[1].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[1].second == "-22");

    CHECK(result.tokens[2].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[2].second == "-333");

    CHECK(result.tokens[3].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[3].second == "-4444");

    CHECK(result.tokens[4].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[4].second == "-55555");

    CHECK(result.tokens[5].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[5].second == "-65537");
  }

  SECTION("Zero") {
    auto result = masm::frontend::tokenize<tokenizer_t>("+0 -0 000  0", lexer);
    CHECK(result.success);
    CHECK(result.rest.empty());
    CHECK(!result.error_message);
    CHECK(result.tokens.size() == 4);

    CHECK(result.tokens[0].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[0].second == "+0");

    CHECK(result.tokens[1].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[1].second == "-0");

    CHECK(result.tokens[2].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[2].second == "000");

    CHECK(result.tokens[3].first == tokenizer_t::token_type::kDecConstant);
    CHECK(result.tokens[3].second == "0");
  }
}
