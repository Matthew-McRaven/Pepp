#include <cstdint>

#include <catch.hpp>

#include "components/storage/block.hpp"
#include "emu/local_machine.hpp"
#include "isa/pep10.hpp"

// Don't do any program simulation here.
TEST_CASE("Test ability to add/remove breakpoints", "[isa::pep10]") {
  SECTION("Can add / remove breakpoints") {
    auto storage = std::make_shared<components::storage::Block<uint16_t, true, uint8_t>>(0xFFFF);
    auto machine = std::make_shared<isa::pep10::LocalMachine<true>>(storage);
    CHECK_NOTHROW(machine->add_breakpoint(0x0000));
    CHECK(machine->remove_breakpoint(0x0000));
    CHECK_FALSE(machine->remove_breakpoint(0x0001));
    CHECK_FALSE(machine->remove_breakpoint(0x0000));
  }

  SECTION("Can remove all breakpoints") {
    auto storage = std::make_shared<components::storage::Block<uint16_t, true, uint8_t>>(0xFFFF);
    auto machine = std::make_shared<isa::pep10::LocalMachine<true>>(storage);
    CHECK_NOTHROW(machine->add_breakpoint(0x0000));
    CHECK_NOTHROW(machine->remove_all_breakpoints());
    CHECK_FALSE(machine->remove_breakpoint(0x0001));
    CHECK_FALSE(machine->remove_breakpoint(0x0000));
  }
}
