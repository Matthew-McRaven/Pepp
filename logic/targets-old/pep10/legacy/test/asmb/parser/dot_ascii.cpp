#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"

TEST_CASE("Parse dot ascii", "[asmb::pep10::parser]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("Valid .ASCII") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "s:asra\n.ASCII \"Hello\"\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_ascii = section->ir_lines[1];
    auto as_ascii = std::dynamic_pointer_cast<ir::dot_ascii<uint16_t>>(maybe_ascii);
    REQUIRE(as_ascii);
  }

  SECTION("Valid .ASCII + comment") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".ASCII \"s\" ;Hi guys\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_ascii = section->ir_lines[0];
    auto as_ascii = std::dynamic_pointer_cast<ir::dot_ascii<uint16_t>>(maybe_ascii);
    REQUIRE(as_ascii);
    REQUIRE(as_ascii->comment);
    CHECK(as_ascii->comment.value() == "Hi guys");
  }

  SECTION("Valid .ASCII + symbol + comment") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "s: .ASCII \"sb\" ;Hi guys\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_ascii = section->ir_lines[0];
    auto as_ascii = std::dynamic_pointer_cast<ir::dot_ascii<uint16_t>>(maybe_ascii);
    REQUIRE(as_ascii);
    REQUIRE(as_ascii->comment);
    REQUIRE(as_ascii->symbol_entry);
    CHECK(as_ascii->symbol_entry->name == "s");
  }

  SECTION("No dec in .ASCII") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".ASCII 22\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No hex in .ASCII") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".ASCII 0xbeef\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No signed dec in .ASCII") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".ASCII -19\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No identifer in .ASCII") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".ASCII HI\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No char in .ASCII") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".ASCII 'HI'\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }
}
