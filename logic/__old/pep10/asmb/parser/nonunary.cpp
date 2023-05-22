#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"

TEST_CASE("Parse nonunary instructions", "[asmb::pep10::parser]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("Nonunary instruction, requires addressing mode.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "ADDA 20, i\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_nonunary = section->ir_lines[0];
    auto as_nonunary = std::dynamic_pointer_cast<asmb::pep10::nonunary_instruction>(maybe_nonunary);
    REQUIRE(as_nonunary);
    CHECK(as_nonunary->mnemonic == isa::pep10::instruction_mnemonic::ADDA);
  }

  SECTION("Nonunary instruction, branches.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "BR 0x20,i\nBR 0x20,x\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 3);
  }

  SECTION("Nonunary instruction, implict addressing mode.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "BR 20\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_nonunary = section->ir_lines[0];
    auto as_nonunary = std::dynamic_pointer_cast<asmb::pep10::nonunary_instruction>(maybe_nonunary);
    REQUIRE(as_nonunary);
    CHECK(as_nonunary->mnemonic == isa::pep10::instruction_mnemonic::BR);
    CHECK(as_nonunary->addressing_mode == isa::pep10::addressing_mode::I);
  }

  SECTION("Nonunary instruction, illegal addr mode.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "LDWT 20, sfx\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }
}
