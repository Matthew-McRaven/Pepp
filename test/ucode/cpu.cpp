/*
 * Copyright (c) 2023-2025 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <catch.hpp>

#include "../../lib/sim3/cores/pep/traced_pep9_mc2.hpp"
#include "bts/microarch/pep.hpp"
#include "toolchain2/ucode/pep_parser.hpp"

namespace {
template <typename CPU> std::pair<sim::memory::Dense<quint16>, CPU> make() {
  sim::api2::device::ID id = 0;
  auto nextID = [&id]() { return id++; };
  auto desc_mem =
      sim::api2::device::Descriptor{.id = nextID(), .compatible = nullptr, .baseName = "dev", .fullName = "/dev"};
  auto span = sim::api2::memory::AddressSpan<quint16>(0, 0xffff);
  sim::memory::Dense<quint16> mem(desc_mem, span, 0);
  auto desc_cpu =
      sim::api2::device::Descriptor{.id = nextID(), .compatible = nullptr, .baseName = "cpu", .fullName = "/cpu"};
  CPU cpu(desc_cpu, nextID);
  return {std::move(mem), std::move(cpu)};
}
template <typename T> quint8 read(sim::api2::memory::Target<T> &mem, T addr) {
  quint8 value = 0;
  auto op = sim::api2::memory::Operation{.type = sim::api2::memory::Operation::Type::Application,
                                         .kind = sim::api2::memory::Operation::Kind::data};
  auto result = mem.read(addr, {&value, 1}, op);
  return value;
}
} // namespace
TEST_CASE("Sanity Tests for 1 Byte ucode", "[scope:mc2][kind:unit][arch:*]") {
  using uarch = pepp::tc::arch::Pep9ByteBus;
  using regs = pepp::tc::arch::Pep9Registers;
  using Code = pepp::tc::arch::Pep9ByteBus::Code;
  using Parser = pepp::tc::parse::MicroParser<uarch, regs>;
  SECTION("Register set preconditions") {
    auto [mem, cpu] = make<targets::pep9::mc2::CPUByteBus>();
    cpu.setTarget(&mem, nullptr);
    QString source = "UnitPre: x=0x2345, mem[0xfffe]=7";
    auto result = Parser(source).parse();
    CHECK(result.program.size() == 1);
    CHECK(read<quint8>(*cpu.bankRegs(), 2) == 0);
    CHECK(read<quint8>(*cpu.bankRegs(), 3) == 0);
    CHECK(read<quint8>(*cpu.bankRegs(), 30) == 0);
    CHECK(read<quint16>(mem, 0xFFFE) == 0);
    cpu.setConstantRegisters();
    cpu.applyPreconditions(result.program[0].tests);
    CHECK(read<quint8>(*cpu.bankRegs(), 2) == 0x23);
    CHECK(read<quint8>(*cpu.bankRegs(), 3) == 0x45);
    CHECK(read<quint8>(*cpu.bankRegs(), 30) == 0xFE);
    CHECK(read<quint16>(mem, 0xFFFE) == 7);
  }
  SECTION("ALU addition") {
    auto [mem, cpu] = make<targets::pep9::mc2::CPUByteBus>();
    cpu.setTarget(&mem, nullptr);
    cpu.setConstantRegisters();
    std::vector<Code> microcode = {Code{
        .A = 28,
        .B = 22,
        .AMux = 1,
        .ALU = 1,
        .CMux = 1,
        .C = 0,
        .NCk = 1,
        .CCk = 1,
    }};
    cpu.setMicrocode(std::move(microcode));
    cpu.clock(1);
    CHECK(read<quint8>(*cpu.bankRegs(), 0) == 0xF0);
    CHECK(read<quint8>(*cpu.csrs(), 0) == 1);
  }
  SECTION("memread") {
    auto [mem, cpu] = make<targets::pep9::mc2::CPUByteBus>();
    cpu.setTarget(&mem, nullptr);
    QString source = "UnitPre: mem[0xFEFF]=0x17";
    auto result = Parser(source).parse();
    CHECK(result.program.size() == 1);
    CHECK(read<quint8>(*cpu.bankRegs(), 0) == 0x0);
    cpu.setConstantRegisters();
    cpu.applyPreconditions(result.program[0].tests);
    auto microcode = std::vector<Code>{{Code{.A = 30, .B = 31, .MARCk = 1}, Code{.MemRead = 1}, Code{.MemRead = 1},
                                        Code{.MemRead = 1, .MDRMux = 0, .MDRCk = 1},
                                        Code{
                                            .AMux = 0,
                                            .ALU = 0,
                                            .CMux = 1,
                                            .C = 0,
                                            .CCk = 1,
                                        }}};
    cpu.setMicrocode(std::move(microcode));
    for (int cycle = 1; cpu.status() != targets::pep9::mc2::CPUByteBus::Status::Halted;) cpu.clock(cycle++);
    CHECK(read<quint8>(*cpu.bankRegs(), 0) == 0x17);
  }
  SECTION("memwrite") {
    auto [mem, cpu] = make<targets::pep9::mc2::CPUByteBus>();
    cpu.setTarget(&mem, nullptr);
    cpu.setConstantRegisters();
    auto microcode = std::vector<Code>{{
        Code{.A = 30, .B = 31, .MARCk = 1},
        Code{.MemWrite = 1, .A = 30, .AMux = 1, .ALU = 0, .CMux = 1, .MDRMux = 1, .MDRCk = 1},
        Code{.MemWrite = 1},
        Code{.MemWrite = 1},
    }};
    cpu.setMicrocode(std::move(microcode));
    CHECK(read<quint16>(mem, 0xFEFF) == 0);
    for (int cycle = 1; cpu.status() != targets::pep9::mc2::CPUByteBus::Status::Halted;) cpu.clock(cycle++);
    CHECK(read<quint16>(mem, 0xFEFF) == 0xFE);
  }
  SECTION("Halted memwrite") {
    auto [mem, cpu] = make<targets::pep9::mc2::CPUByteBus>();
    cpu.setTarget(&mem, nullptr);
    cpu.setConstantRegisters();
    auto microcode =
        std::vector<Code>{{Code{.A = 30, .B = 31, .MARCk = 1},
                           Code{.MemWrite = 1, .A = 30, .AMux = 1, .ALU = 0, .CMux = 1, .MDRMux = 1, .MDRCk = 1},
                           Code{.MemWrite = 1}, Code{.A = 5}}};
    cpu.setMicrocode(std::move(microcode));
    CHECK(read<quint16>(mem, 0xFEFF) == 0);
    for (int cycle = 1; cpu.status() != targets::pep9::mc2::CPUByteBus::Status::Halted;) cpu.clock(cycle++);
    CHECK(read<quint16>(mem, 0xFEFF) == 0x00);
  }
}

TEST_CASE("Sanity Tests for 2 Byte ucode", "[scope:mc2][kind:unit][arch:*]") {
  using uarch = pepp::tc::arch::Pep9WordBus;
  using regs = pepp::tc::arch::Pep9Registers;
  using Code = pepp::tc::arch::Pep9WordBus::Code;
  using Parser = pepp::tc::parse::MicroParser<uarch, regs>;
  SECTION("Register set preconditions") {
    auto [mem, cpu] = make<targets::pep9::mc2::CPUByteBus>();
    cpu.setTarget(&mem, nullptr);
    QString source = "UnitPre: x=0x2345, mem[0xfffe]=7";
    auto result = Parser(source).parse();
    CHECK(result.program.size() == 1);
    CHECK(read<quint8>(*cpu.bankRegs(), 2) == 0);
    CHECK(read<quint8>(*cpu.bankRegs(), 3) == 0);
    CHECK(read<quint8>(*cpu.bankRegs(), 30) == 0);
    CHECK(read<quint16>(mem, 0xFFFE) == 0);
    cpu.setConstantRegisters();
    cpu.applyPreconditions(result.program[0].tests);
    CHECK(read<quint8>(*cpu.bankRegs(), 2) == 0x23);
    CHECK(read<quint8>(*cpu.bankRegs(), 3) == 0x45);
    CHECK(read<quint8>(*cpu.bankRegs(), 30) == 0xFE);
    CHECK(read<quint16>(mem, 0xFFFE) == 7);
  }
  SECTION("ALU addition") {
    auto [mem, cpu] = make<targets::pep9::mc2::CPUWordBus>();
    cpu.setTarget(&mem, nullptr);
    cpu.setConstantRegisters();
    std::vector<Code> microcode = {Code{
        .A = 28,
        .B = 22,
        .AMux = 1,
        .ALU = 1,
        .CMux = 1,
        .C = 0,
        .NCk = 1,
        .CCk = 1,
    }};
    cpu.setMicrocode(std::move(microcode));
    cpu.clock(1);
    CHECK(read<quint8>(*cpu.bankRegs(), 0) == 0xF0);
    CHECK(read<quint8>(*cpu.csrs(), 0) == 1);
  }
  SECTION("memread") {
    auto [mem, cpu] = make<targets::pep9::mc2::CPUWordBus>();
    cpu.setTarget(&mem, nullptr);
    QString source = "UnitPre: mem[0xFEFE]=0x67, mem[0xFEFF]=0x17";
    auto result = Parser(source).parse();
    CHECK(result.program.size() == 1);
    CHECK(read<quint8>(*cpu.bankRegs(), 0) == 0x0);
    cpu.setConstantRegisters();
    cpu.applyPreconditions(result.program[0].tests);
    auto microcode = std::vector<Code>{{Code{
                                            .A = 30,
                                            .B = 31,
                                            .MARMux = 1,
                                            .MARCk = 1,
                                        },
                                        Code{.MemRead = 1}, Code{.MemRead = 1},
                                        Code{
                                            .MemRead = 1,
                                            .MDROMux = 0,
                                            .MDREMux = 0,
                                            .MDROCk = 1,
                                            .MDRECk = 1,
                                        },
                                        Code{
                                            .EOMux = 0,
                                            .AMux = 0,
                                            .ALU = 0,
                                            .CMux = 1,
                                            .C = 0,
                                            .CCk = 1,
                                        },
                                        Code{
                                            .EOMux = 1,
                                            .AMux = 0,
                                            .ALU = 0,
                                            .CMux = 1,
                                            .C = 1,
                                            .CCk = 1,
                                        }}};
    cpu.setMicrocode(std::move(microcode));
    for (int cycle = 1; cpu.status() != targets::pep9::mc2::CPUWordBus::Status::Halted;) cpu.clock(cycle++);
    CHECK(read<quint8>(*cpu.bankRegs(), 0) == 0x67);
    CHECK(read<quint8>(*cpu.bankRegs(), 1) == 0x17);
  }
  SECTION("memwrite") {
    auto [mem, cpu] = make<targets::pep9::mc2::CPUWordBus>();
    cpu.setTarget(&mem, nullptr);
    cpu.setConstantRegisters();
    auto microcode = std::vector<Code>{{
        Code{.A = 30, .B = 31, .MARMux = 1, .MARCk = 1},
        Code{.A = 31, .AMux = 1, .ALU = 0, .CMux = 1, .MDROMux = 1, .MDROCk = 1},
        Code{.MemWrite = 1, .A = 30, .AMux = 1, .ALU = 0, .CMux = 1, .MDREMux = 1, .MDRECk = 1},
        Code{.MemWrite = 1},
        Code{.MemWrite = 1},
    }};
    cpu.setMicrocode(std::move(microcode));
    CHECK(read<quint16>(mem, 0xFEFE) == 0);
    CHECK(read<quint16>(mem, 0xFEFF) == 0);
    for (int cycle = 1; cpu.status() != targets::pep9::mc2::CPUWordBus::Status::Halted;) cpu.clock(cycle++);
    CHECK(read<quint16>(mem, 0xFEFE) == 0xFE);
    CHECK(read<quint16>(mem, 0xFEFF) == 0xFF);
  }
  SECTION("Halted memwrite") {
    auto [mem, cpu] = make<targets::pep9::mc2::CPUWordBus>();
    cpu.setTarget(&mem, nullptr);
    cpu.setConstantRegisters();
    auto microcode =
        std::vector<Code>{{Code{.A = 30, .B = 31, .MARCk = 1},
                           Code{.MemWrite = 1, .A = 30, .AMux = 1, .ALU = 0, .CMux = 1, .MDREMux = 1, .MDRECk = 1},
                           Code{.MemWrite = 1}, Code{.A = 5}}};
    cpu.setMicrocode(std::move(microcode));
    CHECK(read<quint16>(mem, 0xFEFE) == 0);
    CHECK(read<quint16>(mem, 0xFEFF) == 0);
    for (int cycle = 1; cpu.status() != targets::pep9::mc2::CPUWordBus::Status::Halted;) cpu.clock(cycle++);
    CHECK(read<quint16>(mem, 0xFEFE) == 0x00);
    CHECK(read<quint16>(mem, 0xFEFF) == 0x00);
  }
}
