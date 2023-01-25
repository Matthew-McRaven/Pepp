#include "catch.hpp"

#include "asmb/tokenizer.hpp"
#include "asmdr/driver.hpp"
#include "asmdr/frontend/tokenizer.hpp"
#include "asmdr/frontend/tokens.hpp"
#include "asmdr/project/init_project.hpp"
#include "asmdr/project/project.hpp"
#include "macro/registry.hpp"

TEST_CASE("Recognize existing macros", "[asmb::pep10::preprocessor]") {

  using tokenizer_t = asmb::pep10::tokenizer<masm::frontend::lexer_t>;
  using stage_t = masm::project::toolchain_stage;
  using driver_t = masm::driver<uint16_t, stage_t>;
  using project_t = driver_t::group_t;
  using target_t = driver_t::target_t;
  using section_t = driver_t::section_t;
  using result_t = driver_t::result_t;
  using transform_t = driver_t::transform_t;

  driver_t driver;

  masm::frontend::tokenizer<uint16_t, tokenizer_t> tokenizer{};
  transform_t tx_tokenizer = [&](target_t &target,
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

  masm::frontend::preprocessor<uint16_t, tokenizer_t> preproc{};
  transform_t tx_preproc = [&](target_t &target,
                               std::list<driver_t::work_t> &work) {
    bool success = true;
    driver_t::work_iterable_t result_work;
    for (auto value : work) {
      auto [local_success, local_children] =
          preproc.preprocess(target, std::get<driver_t::section_t>(value));
      for (auto child : local_children) {
        result_work.emplace_back(driver_t::work_iterable_t::value_type{
            stage_t::RAW,
            std::static_pointer_cast<ir::section::code_section<uint16_t>>(
                child)});
      }
      result_work.emplace_back(
          driver_t::work_iterable_t::value_type{stage_t::PREPROCESS, value});
      success &= local_success;
    }
    return driver_t::result_t{success, result_work};
  };
  driver.register_transform(tx_preproc, stage_t::TOKEN);

  SECTION("Invoke 0-arity macro.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "LWDA 20,d\n@HELLO0\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;
    file->macro_registry->register_macro("HELLO0", "@HELLO0 0\n.END\n",
                                         masm::MacroType::CoreMacro);

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver.assemble(group, masm::project::toolchain_stage::PREPROCESS);
    CHECK(res.first);
  }

  SECTION("Invoke 1-arty macro with dec constant.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "@HELLO1 01\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;
    file->macro_registry->register_macro("HELLO1", "@HELLO1 1\n.END\n",
                                         masm::MacroType::CoreMacro);

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver.assemble(group, masm::project::toolchain_stage::PREPROCESS);
    CHECK(res.first);
  }

  SECTION("Invoke 1-arty macro with hex constant.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "@HELLO1 0x01\n.END\n";

    file->macro_registry->register_macro("HELLO1", "@HELLO1 1\n.END\n",
                                         masm::MacroType::CoreMacro);
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver.assemble(group, masm::project::toolchain_stage::PREPROCESS);
    CHECK(res.first);
  }

  SECTION("Invoke 1-arty macro with string constant.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "@HELLO1 \"01\"\n.END\n";

    file->macro_registry->register_macro("HELLO1", "@HELLO1 1\n.END\n",
                                         masm::MacroType::CoreMacro);
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver.assemble(group, masm::project::toolchain_stage::PREPROCESS);
    CHECK(res.first);
  }

  SECTION("Invoke 1-arty macro with character constant.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "@HELLO1 '0'\n.END\n";

    file->macro_registry->register_macro("HELLO1", "@HELLO1 1\n.END\n",
                                         masm::MacroType::CoreMacro);
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver.assemble(group, masm::project::toolchain_stage::PREPROCESS);
    CHECK(res.first);
  }

  SECTION("Invoke 1-arity macro with identifier.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "LWDA 20,d\n@HELLO1 hello\n.END\n";

    file->macro_registry->register_macro("HELLO1", "@HELLO1 1\n.END\n",
                                         masm::MacroType::CoreMacro);
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver.assemble(group, masm::project::toolchain_stage::PREPROCESS);
    CHECK(res.first);
  }

  SECTION("No such macro.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "LWDA 20,d\n@HELLO1 hello,world\n.END\n";

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver.assemble(group, masm::project::toolchain_stage::PREPROCESS);

    CHECK_FALSE(res.first);
    CHECK(!res.second.empty());
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    auto errors = target->message_resolver->errors_for_section(section);
    REQUIRE(errors.size() == 1);
    CHECK(std::get<1>(*errors.begin()).message ==
        fmt::vformat(masm::frontend::detail::error_does_not_exist,
                     fmt::make_format_args("HELLO1")));
  }

  SECTION("Invoke 1-arity macro with 2 args.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "LWDA 20,d\n@HELLO1 hello,world\n.END\n";

    file->macro_registry->register_macro("HELLO1", "@HELLO1 1\n.END\n",
                                         masm::MacroType::CoreMacro);
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver.assemble(group, masm::project::toolchain_stage::PREPROCESS);

    CHECK_FALSE(res.first);
    CHECK(!res.second.empty());
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    auto errors = target->message_resolver->errors_for_section(section);
    REQUIRE(errors.size() == 1);
    CHECK(std::get<1>(*errors.begin()).message ==
        fmt::vformat(masm::frontend::detail::error_bad_arg_count,
                     fmt::make_format_args(2, 1)));
  }

  SECTION("Invoke 1-arity macro with 0 args.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "LWDA 20,d\n@HELLO1\n.END\n";
    file->macro_registry->register_macro("HELLO1", "@HELLO1 1\n.END\n",
                                         masm::MacroType::CoreMacro);

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver.assemble(group, masm::project::toolchain_stage::PREPROCESS);

    CHECK_FALSE(res.first);
    CHECK(!res.second.empty());
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    auto errors = target->message_resolver->errors_for_section(section);
    REQUIRE(errors.size() == 1);
    CHECK(std::get<1>(*errors.begin()).message ==
        fmt::vformat(masm::frontend::detail::error_bad_arg_count,
                     fmt::make_format_args(0, 1)));
  }

  SECTION("1-macro inclusion loop") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "LWDA 20,d\n@HELLOA\n.END\n";

    file->macro_registry->register_macro(
        "HELLOA", "@HELLOA 0\n@HELLOA\n.END\n", masm::MacroType::CoreMacro);
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver.assemble(group, masm::project::toolchain_stage::PREPROCESS);

    CHECK_FALSE(res.first);
    CHECK(!res.second.empty());
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    auto errors = target->message_resolver->errors_for_section(section, true);
    REQUIRE(errors.size() == 1);
    CHECK(std::get<1>(*errors.begin()).message ==
        masm::frontend::detail::error_circular_include);
  }

  SECTION("2-macro inclusion loop") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "LWDA 20,d\n@HELLOA\n.END\n";

    file->macro_registry->register_macro(
        "HELLOA", "@HELLOA 0\n@HELLOB\n.END\n", masm::MacroType::CoreMacro);
    file->macro_registry->register_macro(
        "HELLOB", "@HELLOB 0\n@HELLOA\n.END\n", masm::MacroType::CoreMacro);
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver.assemble(group, masm::project::toolchain_stage::PREPROCESS);

    CHECK_FALSE(res.first);
    CHECK(!res.second.empty());
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    auto errors = target->message_resolver->errors_for_section(section, true);
    REQUIRE(errors.size() == 1);
    CHECK(std::get<1>(*errors.begin()).message ==
        masm::frontend::detail::error_circular_include);
  }

  SECTION("4-macro inclusion loop") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "LWDA 20,d\n@HELLOA\n.END\n";

    file->macro_registry->register_macro(
        "HELLOA", "@HELLOA 0\n@HELLOB\n.END\n", masm::MacroType::CoreMacro);
    file->macro_registry->register_macro(
        "HELLOB", "@HELLOB 0\n@HELLOC\n.END\n", masm::MacroType::CoreMacro);
    file->macro_registry->register_macro(
        "HELLOC", "@HELLOC 0\n@HELLOD\n.END\n", masm::MacroType::CoreMacro);
    file->macro_registry->register_macro(
        "HELLOD", "@HELLOD 0\n@HELLOA\n.END\n", masm::MacroType::CoreMacro);
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver.assemble(group, masm::project::toolchain_stage::PREPROCESS);

    CHECK_FALSE(res.first);
    CHECK(!res.second.empty());
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    auto errors = target->message_resolver->errors_for_section(section, true);
    REQUIRE(errors.size() == 1);
    CHECK(std::get<1>(*errors.begin()).message ==
        masm::frontend::detail::error_circular_include);
  }
}
