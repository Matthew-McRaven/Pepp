#include <bit>
#include <cstdint>

#include <catch.hpp>
#include <fmt/core.h>

#include "components/storage/base.hpp"
#include "components/storage/block.hpp"
#include "emu/local_machine.hpp"
#include "emu/local_processor.hpp"
#include "isa/pep10.hpp"

TEST_CASE("Instruction: LDWX,i", "[isa::pep10]") {
    auto storage = std::make_shared<components::storage::Block<uint16_t, false, uint8_t>>(0xFFFF);
    auto machine = std::make_shared<isa::pep10::LocalMachine<false>>(storage);

    // RTL: X ← Oprnd ; N ← X < 0 , Z ← X = 0
    SECTION("LDWX, i") {
        // Loop over non-target status bit combinations to ensure that the instruction does not modify non-target bits.
        for (uint8_t vc = 0; static_cast<uint8_t>(vc) + 1 < 0b1'00; vc++) {
            for (uint16_t opspec = 0; static_cast<uint32_t>(opspec) + 1 < 0x1'0000; opspec++) {
                // Object code for instruction under test.
                std::vector<uint8_t> program = {0x48, static_cast<uint8_t>((opspec >> 8) & 0xff),
                                                static_cast<uint8_t>(opspec & 0xff)};
                machine->clear_all(0, 0, false);
                // Set the starting status bits so that we can check that they are not mutated by this instruction.
                machine->write_csr(isa::pep10::CSR::V, (vc & 0b10 ? 1 : 0));
                machine->write_csr(isa::pep10::CSR::C, (vc & 0b01 ? 1 : 0));
                isa::pep10::load_bytes(machine, program, 0).value();

                auto ret = machine->step();
                REQUIRE(ret.has_value());
                CHECK(ret.value() == step::Result::kNominal);

                // Check that other registers were not mutated.
                CHECK(machine->read_register(isa::pep10::Register::SP) == 0);
                CHECK(machine->read_register(isa::pep10::Register::A) == 0);
                CHECK(machine->read_register(isa::pep10::Register::TR) == 0);
                // Status bits did not change.
                CHECK(machine->read_csr(isa::pep10::CSR::V) == (vc & 0b10 ? 1 : 0));
                CHECK(machine->read_csr(isa::pep10::CSR::C) == (vc & 0b01 ? 1 : 0));
                // PC incremented by 3.
                CHECK(machine->read_register(isa::pep10::Register::PC) == 0x03);
                // IS has the correct instruction mnemonic
                CHECK(machine->read_register(isa::pep10::Register::IS) == 0x48);
                // OS loaded the Mem[0x0001-0x0002].
                CHECK(machine->read_register(isa::pep10::Register::OS) == opspec);
                // Check that target register was had operand loaded.
                CHECK(machine->read_register(isa::pep10::Register::X) == opspec);
                // Check that target status bits match RTL.
                CHECK(machine->read_csr(isa::pep10::CSR::N) == ((opspec & 0x8000) ? 1 : 0));
                CHECK(machine->read_csr(isa::pep10::CSR::Z) == ((opspec == 0) ? 1 : 0));
            }
        }
    }
}
