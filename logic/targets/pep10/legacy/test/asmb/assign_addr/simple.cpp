#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"
#include "ir/empty.hpp"

TEST_CASE("Allocate address to simple programs", "[masm::ir::assign_addr]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();
  // TODO: Restore when address assignment works.
  /*
  SECTION("Unary instructions") {
    auto project = masm::project::init_project<uint16_t>();
    auto file = std::make_shared<masm::project::source_file>();
    file->name = "os";
    file->body = ".BURN 0x0001\nASRA\nNOTX\n.END\n";
    auto res = driver->assemble_os(project, file, masm::project::toolchain_stage::FINISHED);
    std::cout << res.second;
    REQUIRE(res.first);
    auto x = project->image->os;
    REQUIRE(project->image->os->body_ir->ir_lines.size() == 4);
    // Line 0.
    auto maybe_unary0 = project->image->os->body_ir->ir_lines[1];
    auto as_unary0 = std::dynamic_pointer_cast<asmb::pep10::unary_instruction>(maybe_unary0);
    REQUIRE(as_unary0);
    CHECK(as_unary0->mnemonic == isa::pep10::instruction_mnemonic::ASRA);
    CHECK(!as_unary0->comment);
    CHECK(as_unary0->base_address() == 0);
    // Line 1
    auto maybe_unary1 = project->image->os->body_ir->ir_lines[2];
    auto as_unary1 = std::dynamic_pointer_cast<asmb::pep10::unary_instruction>(maybe_unary1);
    REQUIRE(as_unary1);
    CHECK(as_unary1->mnemonic == isa::pep10::instruction_mnemonic::NOTX);
    CHECK(!as_unary1->comment);
    CHECK(as_unary1->base_address() == 1);
  }

  SECTION("Non-unary instructions") {
    auto project = masm::project::init_project<uint16_t>();
    auto file = std::make_shared<masm::project::source_file>();
    file->name = "os";
    file->body = ".BURN 0x0005\nbr main\nmain:ADDA 1,i\n.END\n";
    auto res = driver->assemble_os(project, file, masm::project::toolchain_stage::FINISHED);
    std::cout << res.second;
    REQUIRE(res.first);
    auto x = project->image->os;
    REQUIRE(project->image->os->body_ir->ir_lines.size() == 4);
    // Line 0.
    auto maybe0 = project->image->os->body_ir->ir_lines[1];
    auto as0 = std::dynamic_pointer_cast<asmb::pep10::nonunary_instruction>(maybe0);
    REQUIRE(as0);
    CHECK(as0->mnemonic == isa::pep10::instruction_mnemonic::BR);
    CHECK(!as0->comment);
    CHECK(as0->base_address() == 0);
    // Line 1
    auto maybe1 = project->image->os->body_ir->ir_lines[2];
    auto as1 = std::dynamic_pointer_cast<asmb::pep10::nonunary_instruction>(maybe1);
    REQUIRE(as1);
    CHECK(as1->mnemonic == isa::pep10::instruction_mnemonic::ADDA);
    CHECK(!as1->comment);
    CHECK(as1->base_address() == 3);
  }

  SECTION("Fixed width dot commands instructions") {
    auto project = masm::project::init_project<uint16_t>();
    auto file = std::make_shared<masm::project::source_file>();
    file->name = "os";
    file->body = ".BURN 0x0004\n.WORD 1\n.BYTE 2\n.WORD 0xffff\n.END\n";
    auto res = driver->assemble_os(project, file, masm::project::toolchain_stage::FINISHED);
    std::cout << res.second;
    REQUIRE(res.first);
    auto x = project->image->os;
    REQUIRE(project->image->os->body_ir->ir_lines.size() == 5);
    // Line 0.
    auto maybe0 = project->image->os->body_ir->ir_lines[1];
    CHECK(maybe0->base_address() == 0);
    // Line 1
    auto maybe1 = project->image->os->body_ir->ir_lines[2];
    CHECK(maybe1->base_address() == 2);
    // Line 2
    auto maybe2 = project->image->os->body_ir->ir_lines[3];
    CHECK(maybe2->base_address() == 3);
  }

  SECTION("Multiple ASCII commands") {
    auto project = masm::project::init_project<uint16_t>();
    auto file = std::make_shared<masm::project::source_file>();
    file->name = "os";
    file->body = ".BURN 0x0007\n.ASCII \"hi\"\n.ASCII \"world\"\n.BYTE 2\n.END\n";
    auto res = driver->assemble_os(project, file, masm::project::toolchain_stage::FINISHED);
    std::cout << res.second;
    REQUIRE(res.first);
    auto x = project->image->os;
    REQUIRE(project->image->os->body_ir->ir_lines.size() == 5);
    // Line 0.
    auto maybe0 = project->image->os->body_ir->ir_lines[1];
    CHECK(maybe0->base_address() == 0);
    // Line 1
    auto maybe1 = project->image->os->body_ir->ir_lines[2];
    CHECK(maybe1->base_address() == 2);
    // Line 2
    auto maybe2 = project->image->os->body_ir->ir_lines[3];
    CHECK(maybe2->base_address() == 7);
  }
   */
}
