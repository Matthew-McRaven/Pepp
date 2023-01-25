#include "catch.hpp"

#include "asmb/tokenizer.hpp"
#include "macro/registry.hpp"
#include "asmdr/driver.hpp"
#include "asmdr/frontend/tokenizer.hpp"
#include "asmdr/frontend/tokens.hpp"
#include "asmdr/project/init_project.hpp"
#include "asmdr/project/project.hpp"

TEST_CASE("Driver w/ tokenizer", "[asmb::pep10::tokenizer]") {

  using tokenizer_t = asmb::pep10::tokenizer<masm::frontend::lexer_t>;
  using stage_t = masm::project::toolchain_stage;
  using driver_t = masm::driver<uint16_t, stage_t>;
  using group_t = driver_t::group_t;
  using section_t = driver_t::section_t;
  using target_t = driver_t::target_t;
  using transform_t = driver_t::transform_t;

  SECTION("Tokenize input using driver.", "[masm::driver]") {

    driver_t driver;

    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "LWDA 20,d\n.END\n";

    auto group = masm::project::init_group<uint16_t>({file});

    masm::frontend::tokenizer<uint16_t, tokenizer_t> tokenizer{};
    transform_t tx_tokenizer = [&tokenizer](target_t &target,
                                            std::list<driver_t::work_t> &work) {
      bool success = true;
      driver_t::work_iterable_t result_work;
      std::transform(
          work.begin(), work.end(), std::back_inserter(result_work),
          [&](auto &value) {
            success &=
                tokenizer.tokenize(target, std::get<driver_t::section_t>(value));
            return driver_t::work_iterable_t::value_type{stage_t::TOKEN, value};
          });
      return driver_t::result_t{success, result_work};
    };
    driver.register_transform(tx_tokenizer, stage_t::RAW);

    auto res = driver.assemble(group, masm::project::toolchain_stage::TOKEN);
    CHECK(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    auto tokens = target->section_intermediates[section].tokenized;
    CHECK(tokens);
    REQUIRE(tokens.value().tokens.size() == 2);
    REQUIRE(tokens.value().tokens[0].size() == 5);
    CHECK(tokens.value().tokens[0][0].first ==
        masm::frontend::token_type::kIdentifier);
    CHECK(tokens.value().tokens[0][1].first ==
        masm::frontend::token_type::kDecConstant);
    CHECK(tokens.value().tokens[0][2].first ==
        masm::frontend::token_type::kComma);
    CHECK(tokens.value().tokens[0][3].first ==
        masm::frontend::token_type::kIdentifier);
    CHECK(tokens.value().tokens[0][4].first ==
        masm::frontend::token_type::kEmpty);

    CHECK(tokens.value().tokens[1].size() == 2);
    CHECK(tokens.value().tokens[1][0].first ==
        masm::frontend::token_type::kDotDirective);
    CHECK(tokens.value().tokens[1][1].first ==
        masm::frontend::token_type::kEmpty);
  }
}
