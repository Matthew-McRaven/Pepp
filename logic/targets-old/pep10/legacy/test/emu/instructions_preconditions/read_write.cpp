#include <cstdint>

#include <catch.hpp>

#include "components/storage/block.hpp"
#include "emu/local_machine.hpp"
#include "isa/pep10.hpp"

// Don't do any program simulation here.
TEST_CASE("Simple static tests of Pep/10 computer", "[isa::pep10]") {
  SECTION("Test R/W registers") {
    auto storage = std::make_shared<components::storage::Block<uint16_t, true, uint8_t>>(0xFFFF);
    auto machine = std::make_shared<isa::pep10::LocalMachine<true>>(storage);
    machine->write_register(isa::pep10::Register::A, 0xDEAD);
    machine->write_register(isa::pep10::Register::TR, 0xBEEF);
    CHECK(machine->read_register(isa::pep10::Register::A) == 0xDEAD);
    CHECK(machine->read_register(isa::pep10::Register::TR) == 0xBEEF);
  }SECTION("Test R/W NZVC") {
    auto storage = std::make_shared<components::storage::Block<uint16_t, true, uint8_t>>(0xFFFF);
    auto machine = std::make_shared<isa::pep10::LocalMachine<true>>(storage);
    CHECK(machine->read_csr(isa::pep10::CSR::N) == false);
    CHECK(machine->read_csr(isa::pep10::CSR::C) == false);
    machine->write_csr(isa::pep10::CSR::N, true);
    machine->write_csr(isa::pep10::CSR::C, true);
    CHECK(machine->read_csr(isa::pep10::CSR::N) == true);
    CHECK(machine->read_csr(isa::pep10::CSR::C) == true);
  }
}
