#include <bit>
#include <cstdint>
#include <iostream>

#include <catch.hpp>
#include <fmt/core.h>

#include "components/storage/base.hpp"
#include "components/storage/block.hpp"
#include "emu/local_machine.hpp"
#include "emu/local_processor.hpp"
#include "isa/pep10.hpp"

TEST_CASE("Instruction: CPBA,i", "[isa::pep10]") {
  auto storage = std::make_shared<components::storage::Block<uint16_t, false, uint8_t>>(0xFFFF);
  auto machine = std::make_shared<isa::pep10::LocalMachine<false>>(storage);
  static const auto target_reg = isa::pep10::Register::A;
  // RTL: Y ← A[8 : 15] − byte Oprnd ; N ← Y < 0 , Z ← Y = 0 , V ← 0 , C ← 0
  SECTION("CPBA, i") {
    auto target_reg_values = std::vector<uint16_t>{0x0000, 0x0001, 0x7fff, 0x8000, 0x8FFF, 0xFFFF};
    // Loop over a subset of possible values for the target register.
    for (auto target_reg_val : target_reg_values) {
      for (uint16_t opspec = 0; static_cast<uint32_t>(opspec) + 1 < 0x1'0000; opspec++) {
        // Object code for instruction under test.
        std::vector<uint8_t> program = {0x90, static_cast<uint8_t>((opspec >> 8) & 0xff),
                                        static_cast<uint8_t>(opspec & 0xff)};
        machine->clear_all(0, 0, false);
        machine->write_register(target_reg, target_reg_val);
        isa::pep10::load_bytes(machine, program, 0).value();

        auto ret = machine->step();
        REQUIRE(ret.has_value());
        CHECK(ret.value() == step::Result::kNominal);

        // Check that other registers were not mutated.
        CHECK(machine->read_register(isa::pep10::Register::SP) == 0);
        CHECK(machine->read_register(isa::pep10::Register::X) == 0);
        CHECK(machine->read_register(isa::pep10::Register::TR) == 0);
        // PC incremented by 3.
        CHECK(machine->read_register(isa::pep10::Register::PC) == 0x03);
        // IS has the correct instruction mnemonic
        CHECK(machine->read_register(isa::pep10::Register::IS) == 0x90);
        // OS loaded the Mem[0x0001-0x0002].
        CHECK(machine->read_register(isa::pep10::Register::OS) == opspec);
        // Check that target register had arithmetic performed.
        auto new_target = machine->read_register(target_reg);
        auto difference = (static_cast<uint16_t>(target_reg_val + (~opspec + 1))) & 0xff;
        CHECK(new_target == target_reg_val);
        // Check that target status bits match RTL.
        CHECK(machine->read_csr(isa::pep10::CSR::N) == (difference & 0x80 ? 1 : 0));
        CHECK(machine->read_csr(isa::pep10::CSR::Z) == (difference == 0 ? 1 : 0));
        CHECK(machine->read_csr(isa::pep10::CSR::V) == 0);
        // Don't use bit twiddling here. This validates that my bit twiddles in the CPU are logically
        // equivalent to to carrying into bit 17 of a 32-bit type.
        auto result = static_cast<uint32_t>(target_reg_val) + static_cast<uint16_t>(1 + ~opspec);
        CHECK(machine->read_csr(isa::pep10::CSR::C) == 0);
      }
    }
  }
}
