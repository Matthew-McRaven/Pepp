#include <cstdint>

#include <catch.hpp>

#include "components/storage/base.hpp"
#include "components/storage/block.hpp"
#include "emu/local_machine.hpp"
#include "emu/local_processor.hpp"
#include "isa/pep10.hpp"

TEST_CASE("Instruction: MOVAFLG", "[isa::pep10]") {

  // RTL: NZVC ← A[12 : 15]
  SECTION("MOVAFLG") {
    for (uint8_t end_a = 0; end_a < 0b1'0000; end_a++) {
      auto storage = std::make_shared<components::storage::Block<uint16_t, false, uint8_t>>(0xFFFF);
      auto machine = std::make_shared<isa::pep10::LocalMachine<false>>(storage);
      // Object code for instruction under test.
      std::vector<uint8_t> program = {0x05};
      machine->write_register(isa::pep10::Register::A, (0xDEA0 | end_a));
      isa::pep10::load_bytes(machine, program, 0).value();

      auto ret = machine->step();
      REQUIRE(ret.has_value());
      CHECK(ret.value() == step::Result::kNominal);

      // Check that other registers were not mutated.
      CHECK(machine->read_register(isa::pep10::Register::A) == (0xDEA0 | end_a));
      CHECK(machine->read_register(isa::pep10::Register::X) == 0);
      CHECK(machine->read_register(isa::pep10::Register::OS) == 0);
      CHECK(machine->read_register(isa::pep10::Register::TR) == 0);
      // PC was incremented by one byte
      CHECK(machine->read_register(isa::pep10::Register::PC) == 0x01);
      // IS has the correct instruction mnemonic
      CHECK(machine->read_register(isa::pep10::Register::IS) == 0x5);
      // SP took on the value of A.
      CHECK(machine->read_packed_csr() == end_a);
    }
  }
}
