#include <cstdint>

#include <catch.hpp>
#include <fmt/core.h>

#include "components/storage/base.hpp"
#include "components/storage/block.hpp"
#include "emu/local_machine.hpp"
#include "emu/local_processor.hpp"
#include "isa/pep10.hpp"

TEST_CASE("Instruction: NEGX", "[isa::pep10]") {
    auto storage = std::make_shared<components::storage::Block<uint16_t, false, uint8_t>>(0xFFFF);
    auto machine = std::make_shared<isa::pep10::LocalMachine<false>>(storage);
    // Object code for instruction under test.
    std::vector<uint8_t> program = {0x13};
    // RTL: X ← −X ; N ← X < 0 , Z ← X = 0 , V ← {overflow}
    SECTION("NEGX") {
        // Loop over non-target status bit combinations to ensure that the instruction does not modify non-target bits.
        for (uint8_t c = 0; c <= 0b1; c++) {
            for (uint16_t X = 0; static_cast<uint32_t>(X) + 1 < 0x1'0000; X++) {
                machine->clear_all(0, 0, false);
                // Set the starting status bits so that we can check that they are not mutated by this instruction.
                machine->write_csr(isa::pep10::CSR::C, c);
                machine->write_register(isa::pep10::Register::X, X);
                isa::pep10::load_bytes(machine, program, 0).value();

                auto ret = machine->step();
                REQUIRE(ret.has_value());
                CHECK(ret.value() == step::Result::kNominal);

                // Check that other registers were not mutated.
                CHECK(machine->read_register(isa::pep10::Register::SP) == 0);
                CHECK(machine->read_register(isa::pep10::Register::A) == 0);
                CHECK(machine->read_register(isa::pep10::Register::OS) == 0);
                CHECK(machine->read_register(isa::pep10::Register::TR) == 0);
                // VC bits did not change.
                CHECK(machine->read_csr(isa::pep10::CSR::C) == c);
                // PC was incremented by one byte
                CHECK(machine->read_register(isa::pep10::Register::PC) == 0x01);
                // IS has the correct instruction mnemonic
                CHECK(machine->read_register(isa::pep10::Register::IS) == 0x13);
                // X is the bitwise negation of starting X
                auto new_X = machine->read_register(isa::pep10::Register::X);
                CHECK(new_X == static_cast<uint16_t>(~X + 1));
                CHECK(machine->read_csr(isa::pep10::CSR::N) == ((new_X & 0x8000) ? 1 : 0));
                CHECK(machine->read_csr(isa::pep10::CSR::Z) == ((new_X == 0) ? 1 : 0));
                CHECK(machine->read_csr(isa::pep10::CSR::V) == ((new_X == 0x8000) ? 1 : 0));
            }
        }
    }
}
