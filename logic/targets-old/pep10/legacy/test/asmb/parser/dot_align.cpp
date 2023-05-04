#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"

TEST_CASE("Parse dot align", "[asmb::pep10::parser]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("Valid .ALIGN") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".ALIGN 8\n";
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
    auto as_ascii = std::dynamic_pointer_cast<ir::dot_align<uint16_t>>(maybe_ascii);
    REQUIRE(as_ascii);
  }

  SECTION("Valid .ALIGN + comment") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".ALIGN 2 ;Hi guys\n";
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
    auto as_ascii = std::dynamic_pointer_cast<ir::dot_align<uint16_t>>(maybe_ascii);
    REQUIRE(as_ascii);
    REQUIRE(as_ascii->comment);
    CHECK(as_ascii->comment.value() == "Hi guys");
  }

  SECTION("Valid .ALIGN + symbol + comment") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "s: .ALIGN 4 ;Hi guys\n"; // Self reference is actually okay here, but has no use.
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
    auto as_ascii = std::dynamic_pointer_cast<ir::dot_align<uint16_t>>(maybe_ascii);
    REQUIRE(as_ascii);
    REQUIRE(as_ascii->comment);
    REQUIRE(as_ascii->symbol_entry);
    CHECK(as_ascii->symbol_entry->name == "s");
  }

  SECTION("Non-power-of-2 dec in .ALIGN") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".ALIGN 7\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("Non-power-of-2 hex in .ALIGN") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".ALIGN 0xbeef\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No negative dec in .ALIGN") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".ALIGN -19\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No identifer in .ALIGN") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".ALIGN HI\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No char in .ALIGN") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".ALIGN 'HI'\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No string in .ALIGN") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".ALIGN \"HI\"\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }
}
