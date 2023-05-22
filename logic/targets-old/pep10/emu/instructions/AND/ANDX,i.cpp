#include <bit>
#include <cstdint>

#include <catch.hpp>
#include <fmt/core.h>

#include "components/storage/base.hpp"
#include "components/storage/block.hpp"
#include "emu/local_machine.hpp"
#include "emu/local_processor.hpp"
#include "isa/pep10.hpp"

TEST_CASE("Instruction: ANDX,i", "[isa::pep10]") {
    auto storage = std::make_shared<components::storage::Block<uint16_t, false, uint8_t>>(0xFFFF);
    auto machine = std::make_shared<isa::pep10::LocalMachine<false>>(storage);
    static const auto target_reg = isa::pep10::Register::X;
    // RTL: X ← X ∧ Oprnd ; N ← X < 0 , Z ← X = 0
    SECTION("ANDX, i") {
        auto target_reg_values = std::vector<uint16_t>{0x0000, 0x0001, 0x7fff, 0x8000, 0x8FFF, 0xFFFF};
        // Loop over a subset of possible values for the target register.
        for (auto target_reg_val : target_reg_values) {
            // Loop over non-target status bit combinations to ensure that the instruction does not modify non-target
            // bits.
            for (uint8_t vc = 0; static_cast<uint8_t>(vc) + 1 < 0b100; vc++) {
                for (uint16_t opspec = 0; static_cast<uint32_t>(opspec) + 1 < 0x1'0000; opspec++) {
                    auto start_v = vc & 0b10 ? 1 : 0;
                    auto start_c = vc & 0b01 ? 1 : 0;
                    // Object code for instruction under test.
                    std::vector<uint8_t> program = {0xC8, static_cast<uint8_t>((opspec >> 8) & 0xff),
                                                    static_cast<uint8_t>(opspec & 0xff)};
                    machine->clear_all(0, 0, false);
                    machine->write_register(target_reg, target_reg_val);
                    machine->write_csr(isa::pep10::CSR::V, start_v);
                    machine->write_csr(isa::pep10::CSR::C, start_c);
                    isa::pep10::load_bytes(machine, program, 0).value();

                    auto ret = machine->step();
                    REQUIRE(ret.has_value());
                    CHECK(ret.value() == step::Result::kNominal);

                    // Check that other registers were not mutated.
                    CHECK(machine->read_register(isa::pep10::Register::SP) == 0);
                    CHECK(machine->read_register(isa::pep10::Register::A) == 0);
                    CHECK(machine->read_register(isa::pep10::Register::TR) == 0);
                    CHECK(machine->read_csr(isa::pep10::CSR::V) == start_v);
                    CHECK(machine->read_csr(isa::pep10::CSR::C) == start_c);
                    // PC incremented by 3.
                    CHECK(machine->read_register(isa::pep10::Register::PC) == 0x03);
                    // IS has the correct instruction mnemonic
                    CHECK(machine->read_register(isa::pep10::Register::IS) == 0xC8);
                    // OS loaded the Mem[0x0001-0x0002].
                    CHECK(machine->read_register(isa::pep10::Register::OS) == opspec);
                    // Check that target register had arithmetic performed.
                    auto new_target = machine->read_register(target_reg);
                    CHECK(new_target == static_cast<uint16_t>(target_reg_val & opspec));
                    // Check that target status bits match RTL.
                    CHECK(machine->read_csr(isa::pep10::CSR::N) == (new_target & 0x8000 ? 1 : 0));
                    CHECK(machine->read_csr(isa::pep10::CSR::Z) == (new_target == 0 ? 1 : 0));
                }
            }
        }
    }
}
