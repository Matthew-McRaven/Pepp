#include <bit>
#include <cstdint>

#include <catch.hpp>
#include <fmt/core.h>

#include "components/storage/base.hpp"
#include "components/storage/block.hpp"
#include "emu/local_machine.hpp"
#include "emu/local_processor.hpp"
#include "isa/pep10.hpp"

TEST_CASE("Instruction: SRET", "[isa::pep10]") {
    auto storage = std::make_shared<components::storage::Block<uint16_t, false, uint8_t>>(0xFFFF);
    auto machine = std::make_shared<isa::pep10::LocalMachine<false>>(storage);

    // RTL: NZVC ← Mem[SP][4 : 7] ; A ← Mem[SP + 1] ; X ← Mem[SP + 3] ; PC ← Mem[SP + 5] ; SP ← Mem[SP + 7]
    SECTION("SRET") {
        // Object code for instruction under test.
        std::vector<uint8_t> program = {0x1};
        machine->clear_all(0, 0, false);
        // Set the starting status bits so that we can check that they are mutated properly.
        machine->write_packed_csr(0b0000);
        machine->write_register(isa::pep10::Register::SP, 0xFEE4);
        isa::pep10::load_bytes(machine, program, 0).value();
        auto system_stack_idx = machine->address_from_vector(isa::pep10::MemoryVector::kSystem_Stack);

        // Stat bits
        machine->write_memory(0xFEE4, 0b1111).value();
        // A
        machine->write_memory(0xFEE4 + 1, 0xDE).value();
        machine->write_memory(0xFEE4 + 2, 0xAD).value();
        // X
        machine->write_memory(0xFEE4 + 3, 0xBE).value();
        machine->write_memory(0xFEE4 + 4, 0xEF).value();
        // PC
        machine->write_memory(0xFEE4 + 5, 0xCA).value();
        machine->write_memory(0xFEE4 + 6, 0xFE).value();
        // SP
        machine->write_memory(0xFEE4 + 7, 0x80).value();
        machine->write_memory(0xFEE4 + 8, 0x86).value();

        auto ret = machine->step();
        REQUIRE(ret.has_value());
        CHECK(ret.value() == step::Result::kNominal);

        CHECK(machine->read_packed_csr() == 0b1111);
        CHECK(machine->read_register(isa::pep10::Register::A) == 0xDEAD);
        CHECK(machine->read_register(isa::pep10::Register::X) == 0xBEEF);
        CHECK(machine->read_register(isa::pep10::Register::PC) == 0xCAFE);
        CHECK(machine->read_register(isa::pep10::Register::SP) == 0x8086);

        // IS has the correct instruction mnemonic
        CHECK(machine->read_register(isa::pep10::Register::IS) == 0x1);
    }
}
