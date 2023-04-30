#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"
#include "ir/macro.hpp"

TEST_CASE("Parse macro instructions", "[asmb::pep10::parser]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("0-arity macro.") {

    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "@HELLO0\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;

    CHECK(file->macro_registry->register_macro("HELLO2", "@HELLO2 2\nASRA\nASRA\n", masm::MacroType::CoreMacro));
    CHECK(file->macro_registry->register_macro("HELLO0", "@HELLO0 0\nASRA\nASRA\n", masm::MacroType::CoreMacro));

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);

    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_macro = section->ir_lines[0];
    auto as_macro = std::dynamic_pointer_cast<ir::macro_invocation<uint16_t>>(maybe_macro);
    REQUIRE(as_macro);
    CHECK(as_macro->macro->ir_lines.size() == 2);
    CHECK(!as_macro->comment);
  }

  SECTION("2-arity macro.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "@HELLO2 a,b\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;

    CHECK(file->macro_registry->register_macro("HELLO2", "@HELLO2 2\nASRA\nASRA\n", masm::MacroType::CoreMacro));
    CHECK(file->macro_registry->register_macro("HELLO0", "@HELLO0 0\nASRA\nASRA\n", masm::MacroType::CoreMacro));

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);

    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_macro = section->ir_lines[0];
    auto as_macro = std::dynamic_pointer_cast<ir::macro_invocation<uint16_t>>(maybe_macro);
    REQUIRE(as_macro);
    CHECK(as_macro->macro->ir_lines.size() == 2);
    CHECK(!as_macro->comment);
  }

  SECTION("2-arity macro with a 0-arity macro.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "@HELLO2 a,b\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;

    CHECK(file->macro_registry->register_macro("HELLO2", "@HELLO2 2\n@HELLO0\n@HELLO0\n",
                                               masm::MacroType::CoreMacro));
    CHECK(file->macro_registry->register_macro("HELLO0", "@HELLO0 0\nASRA\nASRA\n", masm::MacroType::CoreMacro));

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);

    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_macro = section->ir_lines[0];
    auto as_macro = std::dynamic_pointer_cast<ir::macro_invocation<uint16_t>>(maybe_macro);
    REQUIRE(as_macro);
    CHECK(as_macro->macro->ir_lines.size() == 2);
    CHECK(!as_macro->comment);
  }

  SECTION("2-arity macro with a 0-arity macro, plus comment.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "@HELLO2 a,b;Hi guys\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;

    CHECK(file->macro_registry->register_macro("HELLO2", "@HELLO2 2\n@HELLO0\n@HELLO0\n",
                                               masm::MacroType::CoreMacro));
    CHECK(file->macro_registry->register_macro("HELLO0", "@HELLO0 0\nASRA\nASRA\n", masm::MacroType::CoreMacro));

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);

    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_macro = section->ir_lines[0];
    auto as_macro = std::dynamic_pointer_cast<ir::macro_invocation<uint16_t>>(maybe_macro);
    REQUIRE(as_macro);
    CHECK(as_macro->macro->ir_lines.size() == 2);
    CHECK(as_macro->comment);
  }

  SECTION("2-arity macro with a 0-arity macro, plus comment & symbol.") {

    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "lingus: @HELLO2 a,b;Hi guys\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;

    CHECK(file->macro_registry->register_macro("HELLO2", "@HELLO2 2\n@HELLO0\n@HELLO0\n",
                                               masm::MacroType::CoreMacro));
    CHECK(file->macro_registry->register_macro("HELLO0", "@HELLO0 0\nASRA\nASRA\n", masm::MacroType::CoreMacro));

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);

    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_macro = section->ir_lines[0];
    auto as_macro = std::dynamic_pointer_cast<ir::macro_invocation<uint16_t>>(maybe_macro);
    REQUIRE(as_macro);
    CHECK(as_macro->macro->ir_lines.size() == 2);
    CHECK(as_macro->comment);
  }
}
