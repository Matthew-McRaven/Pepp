#pragma once
#include <cstdio>
#include "../../../lib/core/isa/riscv/rv_types.hpp"
#include "../../../lib/core/isa/riscv/rvi.hpp"
#include "sim3/systems/notraced_riscv_isa3_system.hpp"

namespace riscv
{
	template<AddressType address_t>
	struct testable_insn {
		const char* name;     // test name
		address_t bits; // the instruction bits
		const int reg;        // which register this insn affects
		const int index;      // test loop index
		address_t initial_value; // start value of register
	};

	template <AddressType address_t>
	static bool
	validate(Machine<address_t>& machine, const testable_insn<address_t>& insn,
			std::function<bool(CPU<address_t>&, const testable_insn<address_t>&)> callback)
	{
		static const address_t MEMBASE = 0x1000;

		const std::array<uint32_t, 1> instr_page = {
			insn.bits
		};

		DecodedExecuteSegment<address_t> &des = machine.cpu.init_execute_area(&instr_page[0], MEMBASE, sizeof(instr_page));
		// jump to page containing instruction
		machine.cpu.jump(MEMBASE);
		// execute instruction
		machine.cpu.reg(insn.reg) = insn.initial_value;
		machine.cpu.step_one();
		// There is a max number of execute segments. Evict the latest to avoid the max limit check
		machine.cpu.memory().evict_execute_segment(des);
		// call instruction validation callback
		if ( callback(machine.cpu, insn) ) return true;
		fprintf(stderr, "Failed test: %s on iteration %d\n", insn.name, insn.index);
		fprintf(stderr, "Register value: 0x%X\n", machine.cpu.reg(insn.reg));
		return false;
	}
}
