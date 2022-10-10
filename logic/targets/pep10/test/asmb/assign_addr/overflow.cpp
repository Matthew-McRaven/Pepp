#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"
#include "ir/empty.hpp"

TEST_CASE("Address overflow.", "[masm::backend::assign_addr]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("Address overflow in OS") {
    // TODO:: Restore when OS address assignment works.
    /*
    auto file = std::make_shared<masm::project::source>();
    file->name = "os";
    file->body = ".BURN 0x0000\nbad:ASRA\nbad:NOTX\n.END\n";
    file->t

    auto res = driver->assemble_os(project, file, masm::project::toolchain_stage::ADDRESS_ASSIGN);
    REQUIRE(!res.first);
     */
  }
}
