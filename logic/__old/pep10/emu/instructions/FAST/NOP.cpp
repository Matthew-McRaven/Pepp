#include <cstdint>

#include <catch.hpp>

#include "components/storage/base.hpp"
#include "components/storage/block.hpp"
#include "emu/local_machine.hpp"
#include "emu/local_processor.hpp"
#include "isa/pep10.hpp"

TEST_CASE("Instruction: NOP", "[isa::pep10]") {
    // Loop over non-target status bit combinations to ensure that the instruction does not modify non-target bits.
    for (uint8_t start_stat = 0; start_stat <= 0b1111; start_stat++) {
        // RTL: {No operation}
        DYNAMIC_SECTION(fmt::format("NOP: NZVC={:b}", start_stat)) {
            auto storage = std::make_shared<components::storage::Block<uint16_t, false, uint8_t>>(0xFFFF);
            auto machine = std::make_shared<isa::pep10::LocalMachine<false>>(storage);
            // Object code for instruction under test.
            std::vector<uint8_t> program = {0x08};
            // Set the starting status bits so that we can check that they are not mutated by this instruction.
            machine->write_packed_csr(start_stat);
            isa::pep10::load_bytes(machine, program, 0).value();

            auto ret = machine->step();
            REQUIRE(ret.has_value());
            CHECK(ret.value() == step::Result::kNominal);

            // Check that other registers were not mutated.
            CHECK(machine->read_register(isa::pep10::Register::SP) == 0);
            CHECK(machine->read_register(isa::pep10::Register::X) == 0);
            CHECK(machine->read_register(isa::pep10::Register::OS) == 0);
            CHECK(machine->read_register(isa::pep10::Register::TR) == 0);
            CHECK(machine->read_register(isa::pep10::Register::A) == 0);
            // Status bits did not change.
            CHECK(machine->read_packed_csr() == start_stat);
            // PC was incremented by one byte
            CHECK(machine->read_register(isa::pep10::Register::PC) == 0x01);
            // IS has the correct instruction mnemonic
            CHECK(machine->read_register(isa::pep10::Register::IS) == 0x8);
        }
    }
}
