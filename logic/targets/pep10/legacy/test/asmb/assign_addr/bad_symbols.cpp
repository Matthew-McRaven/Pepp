#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"
#include "ir/empty.hpp"

TEST_CASE("Test for multiple / undefined symbols", "[masm::ir::assign_addr]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  // TODO: Requires working address assignment
  /*
  SECTION("Multiple defined symbol") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "os";
    file->body = ".BURN 0x0001\nbad:ASRA\nbad:NOTX\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::ADDRESS_ASSIGN);
    REQUIRE(!res.first);
  }*/
  // TODO: Now a linker error!!
  /*
  SECTION("Undefined symbol") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "os";
    file->body = ".BURN 0x0002\nbr bad\n.END\n";
    file->target_type = ir::section::section_type::kUserProgram;
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::ADDRESS_ASSIGN);
    REQUIRE(!res.first);
  }*/
}
