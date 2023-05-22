#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"

TEST_CASE("Parse dot equate", "[asmb::pep10::parser]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("decimal .EQUATE") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "s:.EQUATE 33\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_word = section->ir_lines[0];
    auto as_word = std::dynamic_pointer_cast<ir::dot_equate<uint16_t>>(maybe_word);
    REQUIRE(as_word->argument->value() == 33);
  }

  SECTION("signed decimal .EQUATE") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "s:.EQUATE -33\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_word = section->ir_lines[0];
    auto as_word = std::dynamic_pointer_cast<ir::dot_equate<uint16_t>>(maybe_word);
    REQUIRE(as_word->argument->value() == static_cast<uint16_t>(-33));
  }

  SECTION("no symbolic .EQUATE") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "s:.EQUATE 33\nk:.EQUATE s\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("hex .EQUATE") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "s:.EQUATE 0x21\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_word = section->ir_lines[0];
    auto as_word = std::dynamic_pointer_cast<ir::dot_equate<uint16_t>>(maybe_word);
    REQUIRE(as_word->argument->value() == 33);
  }

  SECTION("char .EQUATE") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "s:.EQUATE '!'\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_word = section->ir_lines[0];
    auto as_word = std::dynamic_pointer_cast<ir::dot_equate<uint16_t>>(maybe_word);
    REQUIRE(as_word->argument->value() == 33);
  }

  SECTION("string .EQUATE") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "s:.EQUATE \"!\"\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_word = section->ir_lines[0];
    auto as_word = std::dynamic_pointer_cast<ir::dot_equate<uint16_t>>(maybe_word);
    REQUIRE(as_word->argument->value() == 33);
  }

  SECTION("No 3 byte argument") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "s:.EQUATE \"!!!\"\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }
}
