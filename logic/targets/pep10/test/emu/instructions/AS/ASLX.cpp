#include <bit>
#include <cstdint>

#include <catch.hpp>
#include <fmt/core.h>

#include "components/storage/base.hpp"
#include "components/storage/block.hpp"
#include "emu/local_machine.hpp"
#include "emu/local_processor.hpp"
#include "isa/pep10.hpp"


TEST_CASE("Instruction: ASLX", "[isa::pep10]")
{
	auto storage = std::make_shared<components::storage::Block<uint16_t, false, uint8_t>>(0xFFFF);
	auto machine = std::make_shared<isa::pep10::LocalMachine<false>>(storage);
	// Object code for instruction under test.
	std::vector<uint8_t> program = {0x15};
	// RTL: C ← X[0] , X[0 : 14] ← X[1 : 15] , X[15] ← 0 ; N ← X < 0 , Z ← X = 0 , V ← {overflow}
	SECTION("ASLX")
	{
		for(uint16_t X=0; static_cast<uint32_t>(X)+1<0x1'0000;X++)
		{
			machine->clear_all(0, 0, false);
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
			// PC was incremented by one byte
			CHECK(machine->read_register(isa::pep10::Register::PC) == 0x01);
			// IS has the correct instruction mnemonic
			CHECK(machine->read_register(isa::pep10::Register::IS) == 0x15);
			auto new_X = machine->read_register(isa::pep10::Register::X);
			// Count the number of 1's in X[0:1].
			// If it is 0 or 2, then they agree in sign -> no signed overflow.
			// Otherwise, they disagree in sign -> signed overflow.
			auto top_2_bits = std::popcount(static_cast<uint16_t>(X>>14));
			// X is the starting X, shifted left by 1 bit.
			CHECK(new_X == static_cast<uint16_t>(X<<1));
			CHECK(machine->read_csr(isa::pep10::CSR::N) == ((new_X & 0x8000) ? 1 : 0));
			CHECK(machine->read_csr(isa::pep10::CSR::Z) == ((new_X == 0) ? 1 : 0));
			// Signed overflow if the original top two bits contain opposite values.
			CHECK(machine->read_csr(isa::pep10::CSR::V) == ((top_2_bits % 2) ? 1 : 0));
			// Carry out if high order bit was non-zero
			CHECK(machine->read_csr(isa::pep10::CSR::C) == ((X & 0x8000) ? 1 : 0));
		}
	}


}
