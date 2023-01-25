#include <bit>
#include <cstdint>

#include <catch.hpp>
#include <fmt/core.h>

#include "components/storage/base.hpp"
#include "components/storage/block.hpp"
#include "emu/local_machine.hpp"
#include "emu/local_processor.hpp"
#include "isa/pep10.hpp"

TEST_CASE("Instruction: USCALL", "[isa::pep10]") {
    auto storage = std::make_shared<components::storage::Block<uint16_t, false, uint8_t>>(0xFFFF);
    auto machine = std::make_shared<isa::pep10::LocalMachine<false>>(storage);

    // RTL: Y ← Mem[FFF0] ; Mem[Y − 2] ← SP ; Mem[Y − 4] ← PC ; Mem[Y − 6] ← X ;
    // Mem[Y − 8] ← A ; Mem[Y − 9][4 : 7] ← NZVC ; SP ← Y − 9 ; PC ← Mem[FFFE]
    SECTION("USCALL") {
        // Object code for instruction under test.
        std::vector<uint8_t> program = {0x7};
        for (uint8_t start_stat = 0; start_stat + 1 < 0b1'0000; start_stat++) {
            machine->clear_all(0, 0, false);
            // Set the starting status bits so that we can check that they are not mutated by this instruction.
            machine->write_packed_csr(start_stat);
            machine->write_register(isa::pep10::Register::SP, 0xDEAD);
            machine->write_register(isa::pep10::Register::A, 0xBEEF);
            machine->write_register(isa::pep10::Register::X, 0xCAFE);
            isa::pep10::load_bytes(machine, program, 0).value();
            auto trap_handler = machine->address_from_vector(isa::pep10::MemoryVector::kTrap_Handler);
            auto system_stack = machine->address_from_vector(isa::pep10::MemoryVector::kSystem_Stack);
            machine->write_memory(system_stack, 0xFE).value();
            machine->write_memory(system_stack + 1, 0xED).value();
            machine->write_memory(trap_handler, 0xD0).value();
            machine->write_memory(trap_handler + 1, 0x0D).value();

            auto ret = machine->step();
            REQUIRE(ret.has_value());
            CHECK(ret.value() == step::Result::kNominal);

            CHECK(machine->read_register(isa::pep10::Register::SP) == 0xFEED - 10);
            CHECK(machine->read_register(isa::pep10::Register::PC) == 0xD00D);
            CHECK((int)machine->get_memory(0xFEED - 1).value() == 0x7);
            CHECK((int)machine->get_memory(0xFEED - 3).value() == 0xDE);
            CHECK((int)machine->get_memory(0xFEED - 2).value() == 0xAD);
            CHECK((int)machine->get_memory(0xFEED - 5).value() == 0x00);
            CHECK((int)machine->get_memory(0xFEED - 4).value() == 0x01);
            CHECK((int)machine->get_memory(0xFEED - 7).value() == 0xCA);
            CHECK((int)machine->get_memory(0xFEED - 6).value() == 0xFE);
            CHECK((int)machine->get_memory(0xFEED - 9).value() == 0xBE);
            CHECK((int)machine->get_memory(0xFEED - 8).value() == 0xEF);
            CHECK((int)machine->get_memory(0xFEED - 10).value() == start_stat);
            // Trap register must not be mutated.
            CHECK(machine->read_register(isa::pep10::Register::TR) == 0);
            // IS has the correct instruction mnemonic
            CHECK(machine->read_register(isa::pep10::Register::IS) == 0x7);
        }
    }
}
