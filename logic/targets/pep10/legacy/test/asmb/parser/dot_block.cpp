#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"

TEST_CASE("Parse dot block", "[asmb::pep10::parser]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("decimal .BLOCK") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".BLOCK 33\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_block = section->ir_lines[0];
    auto as_block = std::dynamic_pointer_cast<ir::dot_block<uint16_t>>(maybe_block);
    REQUIRE(as_block->object_code_bytes() == 33);
  }

  SECTION("signed decimal .BLOCK") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".BLOCK -33\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_block = section->ir_lines[0];
    auto as_block = std::dynamic_pointer_cast<ir::dot_block<uint16_t>>(maybe_block);
    REQUIRE(as_block->object_code_bytes() == static_cast<uint16_t>(-33));
  }

  SECTION("hex .BLOCK") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".BLOCK 0x21\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_block = section->ir_lines[0];
    auto as_block = std::dynamic_pointer_cast<ir::dot_block<uint16_t>>(maybe_block);
    REQUIRE(as_block->object_code_bytes() == 33);
  }

  SECTION("no char .BLOCK") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".BLOCK '!'\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("no string .BLOCK") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".BLOCK \"!\"\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }
}
