#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"

TEST_CASE("Parse unary instructions", "[asmb::pep10::parser]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("Unary instruction.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "ASRA\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_unary = section->ir_lines[0];
    auto as_unary = std::dynamic_pointer_cast<asmb::pep10::unary_instruction>(maybe_unary);
    REQUIRE(as_unary);
    CHECK(as_unary->mnemonic == isa::pep10::instruction_mnemonic::ASRA);
    CHECK(!as_unary->comment);
  }

  SECTION("Unary instruction with symbol.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "maybe: ASRA\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_unary = section->ir_lines[0];
    auto as_unary = std::dynamic_pointer_cast<asmb::pep10::unary_instruction>(maybe_unary);
    REQUIRE(as_unary);
    CHECK(as_unary->mnemonic == isa::pep10::instruction_mnemonic::ASRA);
    CHECK(!as_unary->comment);
    REQUIRE(as_unary->symbol_entry);
    CHECK(as_unary->symbol_entry->name == "maybe");
  }

  SECTION("Unary instruction with symbol + comment.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "maybe: ASRA;A comment\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_unary = section->ir_lines[0];
    auto as_unary = std::dynamic_pointer_cast<asmb::pep10::unary_instruction>(maybe_unary);
    REQUIRE(as_unary);
    CHECK(as_unary->mnemonic == isa::pep10::instruction_mnemonic::ASRA);
    REQUIRE(as_unary->comment);
    CHECK(as_unary->comment.value() == "A comment");
    REQUIRE(as_unary->symbol_entry);
    CHECK(as_unary->symbol_entry->name == "maybe");
  }

  SECTION("Unary instruction with comment.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "ASRA ;Hello\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_unary = section->ir_lines[0];
    auto as_unary = std::dynamic_pointer_cast<asmb::pep10::unary_instruction>(maybe_unary);
    REQUIRE(as_unary);
    CHECK(as_unary->mnemonic == isa::pep10::instruction_mnemonic::ASRA);
    REQUIRE(as_unary->comment);
    CHECK(as_unary->comment.value() == "Hello");
    REQUIRE(!as_unary->symbol_entry);
  }
}
