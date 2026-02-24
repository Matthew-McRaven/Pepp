#pragma once
#include "core/arch/pep/uarch/pep.hpp"
#include "core/langs/ucode/pep_ir.hpp"
namespace pepp {
using P9Regs = pepp::tc::arch::Pep9Registers;
using P9ByteBus = pepp::tc::arch::Pep9ByteBus;
using P9WordBus = pepp::tc::arch::Pep9WordBus;
using OneByteMC9 = std::vector<typename P9ByteBus::CodeWithEnables>;
using TwoByteMC9 = std::vector<typename P9WordBus::CodeWithEnables>;
using MicrocodeChoice = std::variant<std::monostate, OneByteMC9, TwoByteMC9>;
using P9Tests = std::vector<pepp::tc::ir::Test<pepp::tc::arch::Pep9Registers>>;
using TestChoice = std::variant<std::monostate, P9Tests>;
} // namespace pepp
