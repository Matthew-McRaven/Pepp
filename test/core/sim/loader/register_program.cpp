/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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
#include <array>
#include <catch.hpp>

#include "core/arch/pep/isa/pep10.hpp"
#include "core/formats/elf/packed_input_group.hpp"
#include "core/formats/elf/packed_io.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/sim/api/loadable.hpp"
#include "core/sim/cores/cpu/pep/pep_isa.hpp"
#include "core/sim/loader.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

namespace {

// A Pep/10 core with one RAM behind it, as make_cpu does elsewhere.
auto make_cpu() {
  PepISA3CPU::Configuration cpu_cfg{
      Device::Configuration{.basename = "cpu", .compatible = PepISA3CPU::compatible},
      PepISA3CPU::ISA::Pep10,
      "/memory",
  };
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  Dense::Configuration mem_cfg{
      Device::Configuration{.basename = "memory", .compatible = Dense::compatible},
      0x00,
      AddressSpan(0x0000, 0xffff),
  };
  auto system = std::make_unique<System>(root_cfg);
  auto *mem = system->make_device<Dense>(mem_cfg);
  auto *cpu = system->make_device<PepISA3CPU>(cpu_cfg, system.get());
  system->initialize();
  return std::make_tuple(std::move(system), mem, cpu);
}

const Operation app(Operation::Type::Application, Operation::Kind::data);

// The vectors are big-endian in memory, while the register bank is host-ordered.
void poke_be(Target *mem, Address at, u16 v) {
  const std::array<u8, 2> bytes{(u8)(v >> 8), (u8)(v & 0xFF)};
  mem->write(at, {bytes.data(), bytes.size()}, app);
}
const bool swap = bits::hostOrder() != bits::Order::BigEndian;

using MV = isa::Pep10::MemoryVectors;

// A Pep/10 ELF with one executable PT_LOAD holding code at base, serialized in memory rather than to disk.
pepp::bts::AnyElfGroup elf_at(Address base, const std::vector<u8> &code) {
  using namespace bits;
  using namespace pepp::bts;
  PackedGrowableElfFile<ElfBits::b32, ElfEndian::be> elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10,
                                                         ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  const auto text = add_named_section(elf, ".text", SectionTypes::SHT_PROGBITS);
  elf.section_data[text]->append(bits::span<const u8>{code});
  elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R | SegmentFlags::PF_X);
  const std::vector<SegmentLayoutConstraint> constraints{
      {.alignment = 1, .from_sec = text, .to_sec = text, .base_address = base}};
  return to_input_group(to_input_elf(elf, &constraints));
}

} // namespace

TEST_CASE("Loader: initial register programming", "[scope:core][scope:core.sim][kind:int][arch:pep10]") {
  auto [sys, mem, cpu] = make_cpu();
  auto *scan = sys->register_scan();
  // Provide dummy values for the vectors so that we can check it the load actually occurs.
  poke_be(mem, (Address)MV::Dispatcher, 0xBEEF);
  poke_be(mem, (Address)MV::SystemStackPtr, 0xFAB0);
  const auto pc = *scan->find("/cpu:PC"), sp = *scan->find("/cpu:SP"), a = *scan->find("/cpu:A");

  SECTION("Load constants and from memory") {
    Loader loader(sys.get());
    REQUIRE(loader.set_register(a, 0x1234));
    REQUIRE(loader.copy_word(pc, mem->id(), (Address)MV::Dispatcher, swap));
    REQUIRE(loader.run());
    CHECK(loader.stop_cause() == tvm::StopCause::None);
    CHECK(scan->read<u16>(a) == 0x1234);
    CHECK(scan->read<u16>(pc) == 0xBEEF);
  }
  SECTION("Memory read at program execution time") {
    Loader loader(sys.get());
    REQUIRE(loader.copy_word(pc, mem->id(), (Address)MV::Dispatcher, swap));
    // Whatever the memory location held when the program was authored is ignored.
    poke_be(mem, (Address)MV::Dispatcher, 0x0BAD);
    REQUIRE(loader.run());
    CHECK(scan->read<u16>(pc) == 0x0BAD);
  }
  SECTION("Disallow invalid register references") {
    Loader loader(sys.get());
    const RegisterScan::RegisterRef nowhere{RegisterScan::Register::ID{0xFFFF}, RegisterScan::Register::Field::ID{0}};
    CHECK_FALSE(loader.set_register(nowhere, 1));
    CHECK_FALSE(loader.copy_word(nowhere, mem->id(), (Address)MV::Dispatcher));
  }
  SECTION("Load an ELF segment into memory") {
    const std::vector<u8> code{0x12, 0x34, 0x56};
    Loader loader(sys.get());
    loader.add_group(elf_at(0x0200, code), cpu->id());
    REQUIRE(loader.run());
    CHECK(loader.stop_cause() == tvm::StopCause::None);
    std::array<u8, 3> actual{};
    mem->read(0x0200, {actual.data(), actual.size()}, app);
    CHECK(std::vector<u8>(actual.begin(), actual.end()) == code);
  }
  SECTION("Invalid memory access stops the loader") {
    Loader from_memory(sys.get());
    REQUIRE(from_memory.copy_word(pc, mem->id(), 0x1'0000, swap));
    CHECK_FALSE(from_memory.run());
    CHECK(from_memory.stop_cause() == tvm::StopCause::AccessRefused);

    Loader segment(sys.get());
    segment.add_group(elf_at(0xFFFE, {0x12, 0x34, 0x56}), cpu->id());
    CHECK_FALSE(segment.run());
    CHECK(segment.stop_cause() == tvm::StopCause::AccessRefused);
  }
  SECTION("Pep/10 initializes SP/PC from memory vectors") {
    Loader loader(sys.get());
    auto *loadable = cpu->capability<Loadable>();
    REQUIRE(loadable != nullptr);
    loadable->register_core_init(loader);
    REQUIRE(loader.run());
    CHECK(scan->read<u16>(pc) == 0xBEEF);
    CHECK(scan->read<u16>(sp) == 0xFAB0);

    // test that we can re-run an existing program.
    poke_be(mem, (Address)MV::Dispatcher, 0x0100);
    REQUIRE(loader.run());
    CHECK(scan->read<u16>(pc) == 0x0100);
    CHECK(scan->read<u16>(sp) == 0xFAB0);
  }
}
