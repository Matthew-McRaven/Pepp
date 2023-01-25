#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"
#include "ir/empty.hpp"
#include "asmdr/elf/pack.hpp"

TEST_CASE("Flatten simple programs to ELF binary.", "[masm::elf]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();
  // TODO: Implement when elf packing is stable.
  /*
  SECTION("Unary instructions") {
    auto project = masm::project::init_project<uint16_t>();
    auto file = std::make_shared<masm::project::source_file>();
    file->name = "main";
    file->body = ".BURN 0xFF00\nASRA\nNOTX\n.END\n";
    auto res = driver->assemble_os(project, file, masm::project::toolchain_stage::PACK);
    REQUIRE(res.first);
  }

  SECTION("Non-unary instructions") {
    auto project = masm::project::init_project<uint16_t>();
    auto file = std::make_shared<masm::project::source_file>();
    file->name = "main";
    file->body = ".BURN 0xFF00\nbr main\nmain:ADDA 1,i\n.END\n";
    auto res = driver->assemble_os(project, file, masm::project::toolchain_stage::PACK);
    REQUIRE(res.first);
  }

  SECTION("Fixed width dot commands instructions") {
    auto project = masm::project::init_project<uint16_t>();
    auto file = std::make_shared<masm::project::source_file>();
    file->name = "main";
    file->body = ".BURN 0xFF00\n.WORD 1\n.BYTE 2\n.WORD 0xffff\n.END\n";
    auto res = driver->assemble_os(project, file, masm::project::toolchain_stage::PACK);
    REQUIRE(res.first);
  }

  SECTION("Multiple ASCII commands") {
    auto project = masm::project::init_project<uint16_t>();
    auto file = std::make_shared<masm::project::source_file>();
    file->name = "main";
    file->body = ".BURN 0xFF00\na:.ASCII \"hi\"\nb:.ASCII \"world\"\nc:.BYTE 2\n.END\n";
    auto res = driver->assemble_os(project, file, masm::project::toolchain_stage::PACK);
    REQUIRE(res.first);
  }*/
}
