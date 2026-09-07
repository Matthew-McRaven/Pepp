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
#include <catch.hpp>
#include <elfio/elfio.hpp>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "core/formats/elf/packed_ops.hpp"
#include "core/formats/elf/packed_types.hpp"

namespace {
using namespace pepp::bts;
using Packed = PackedGrowableElfLE32;

// Appends bytes of filler so a section has something to place.
u16 add_data_section(Packed &elf, const char *name, u32 addralign, size_t size) {
  auto idx = add_named_section(elf, name, SectionTypes::SHT_PROGBITS);
  elf.section_headers[idx].sh_flags = bits::to_underlying(SectionFlags::SHF_ALLOC);
  elf.section_headers[idx].sh_addralign = addralign;
  const std::vector<u8> filler(size, 0xAA);
  elf.section_data[idx]->append(bits::span<const u8>{filler.data(), filler.size()});
  return idx;
}
} // namespace

TEST_CASE("calculate_layout validation", "[scope:elf][kind:unit][arch:*]") {
  using namespace pepp::bts;
  using namespace bits;
  Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  // Deliberately mismatched sizes and alignments to ensure padding is required.
  auto text = add_data_section(elf, ".text", 1, 5);
  auto data = add_data_section(elf, ".data", 8, 4);
  auto rodata = add_data_section(elf, ".rodata", 4, 3);
  elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R | SegmentFlags::PF_W);

  SegmentLayoutConstraint load;
  load.alignment = 4096;
  load.from_sec = text;
  load.to_sec = rodata;
  load.base_address = 0x1000;
  std::vector<SegmentLayoutConstraint> constraints = {load};
  auto layout = calculate_layout(elf, &constraints);
  const auto &phdr = elf.program_headers[0];

  SECTION("Each section's address fulfills its own sh_addralign") {
    for (auto idx : {text, data, rodata}) {
      INFO("section " << idx);
      const auto &shdr = elf.section_headers[idx];
      const u64 align = std::max<u64>(shdr.sh_addralign, 1);
      CHECK(shdr.sh_addr % align == 0);
    }
  }

  SECTION("Ensure sh_offset - p_offset == sh_addr - p_vaddr") {
    for (auto idx : {text, data, rodata}) {
      INFO("section " << idx);
      const auto &shdr = elf.section_headers[idx];
      CHECK(shdr.sh_offset - phdr.p_offset == shdr.sh_addr - phdr.p_vaddr);
    }
  }

  SECTION("p_offset % p_align == p_vaddr % p_align") {
    REQUIRE(phdr.p_align == 4096);
    CHECK(phdr.p_offset % phdr.p_align == phdr.p_vaddr % phdr.p_align);
  }

  SECTION("The null section is empty") {
    CHECK(elf.section_headers[0].sh_offset == 0);
    CHECK(elf.section_headers[0].sh_size == 0);
  }

  SECTION("ELFIO can read our placed object file") {
    std::vector<u8> bytes(size_for_layout(layout), 0);
    write(bytes, layout);
    ELFIO::elfio reader;
    std::istringstream in(std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size()));
    REQUIRE(reader.load(in));
    REQUIRE(reader.segments.size() == 1);
    const auto *seg = reader.segments[0];
    CHECK(seg->get_offset() % seg->get_align() == seg->get_virtual_address() % seg->get_align());
    for (const char *name : {".text", ".data", ".rodata"}) {
      INFO(name);
      const auto *sec = reader.sections[name];
      REQUIRE(sec != nullptr);
      CHECK(sec->get_address() % std::max<ELFIO::Elf_Xword>(sec->get_addr_align(), 1) == 0);
      CHECK(sec->get_offset() - seg->get_offset() == sec->get_address() - seg->get_virtual_address());
    }
  }
}

TEST_CASE("calculate_layout sizes segments to fit sections", "[scope:elf][kind:unit][arch:*]") {
  using namespace pepp::bts;
  using namespace bits;
  Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  auto text = add_data_section(elf, ".text", 1, 4);

  SECTION("p_memsz is not rounded up") {
    auto bss = add_named_section(elf, ".bss", SectionTypes::SHT_NOBITS);
    elf.section_headers[bss].sh_size = 0x100;
    elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R | SegmentFlags::PF_W);

    SegmentLayoutConstraint load;
    load.alignment = 4096;
    load.from_sec = text;
    load.to_sec = bss;
    load.base_address = 0x1000;
    std::vector<SegmentLayoutConstraint> constraints = {load};
    calculate_layout(elf, &constraints);

    CHECK(elf.program_headers[0].p_filesz == 4);
    CHECK(elf.program_headers[0].p_memsz == 4 + 0x100);
  }

  SECTION("p_memsz is set for all segment types") {
    elf.add_segment(SegmentType::PT_NOTE, SegmentFlags::PF_R);
    SegmentLayoutConstraint note;
    note.alignment = 4;
    note.from_sec = text;
    note.to_sec = text;
    note.base_address = 0x1000;
    std::vector<SegmentLayoutConstraint> constraints = {note};
    calculate_layout(elf, &constraints);

    CHECK(elf.program_headers[0].p_filesz == 4);
    CHECK(elf.program_headers[0].p_memsz == 4);
  }

  SECTION("A constraint starting at section 0 leaves segment header unmodified") {
    // from_sec of 0 is how a caller says it filled in this program header itself, so every field must
    // survive layout untouched -- section 0 is SHN_UNDEF, so no real segment can start there.
    Packed::Phdr authored;
    authored.p_type = bits::to_underlying(SegmentType::PT_LOAD);
    authored.p_flags = bits::to_underlying(SegmentFlags::PF_R);
    authored.p_align = 1;
    authored.p_offset = 0x40;
    authored.p_filesz = 0x20;
    authored.p_memsz = 0x1000;
    authored.p_vaddr = authored.p_paddr = 0xFEEDBEEF;
    elf.add_segment(std::move(authored));

    SegmentLayoutConstraint skipped;
    skipped.from_sec = 0;
    std::vector<SegmentLayoutConstraint> constraints = {skipped};
    REQUIRE_NOTHROW(calculate_layout(elf, &constraints));

    const auto &phdr = elf.program_headers[0];
    CHECK(phdr.p_vaddr == 0xFEEDBEEF);
    CHECK(phdr.p_paddr == 0xFEEDBEEF);
    CHECK(phdr.p_offset == 0x40);
    CHECK(phdr.p_filesz == 0x20);
    CHECK(phdr.p_memsz == 0x1000);
    CHECK(phdr.p_align == 1);
  }

  SECTION("A segment can be nested entirely inside a previous segment") {
    auto data = add_data_section(elf, ".data", 1, 4);
    elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R | SegmentFlags::PF_W);
    elf.add_segment(SegmentType::PT_NOTE, SegmentFlags::PF_R);

    SegmentLayoutConstraint load;
    load.alignment = 4096;
    load.from_sec = text;
    load.to_sec = data;
    load.base_address = 0x1000;
    // Same sections, but technically loadable at a different base address.
    // Only the address from the first constraint will apply
    SegmentLayoutConstraint note = load;
    note.alignment = 4;
    note.base_address = 0x9000;
    std::vector<SegmentLayoutConstraint> constraints = {load, note};
    REQUIRE_NOTHROW(calculate_layout(elf, &constraints));

    CHECK(elf.section_headers[text].sh_addr == 0x1000);
    CHECK(elf.program_headers[1].p_vaddr == elf.program_headers[0].p_vaddr);
    CHECK(elf.program_headers[1].p_offset == elf.program_headers[0].p_offset);
    CHECK(elf.program_headers[1].p_memsz == elf.program_headers[0].p_memsz);
  }
}

TEST_CASE("calculate_layout rejects unsatisfiable constraints", "[scope:elf][kind:unit][arch:*][!throws]") {
  using namespace pepp::bts;
  using namespace bits;
  Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  auto text = add_data_section(elf, ".text", 1, 4);
  auto data = add_data_section(elf, ".data", 1, 4);
  auto rodata = add_data_section(elf, ".rodata", 1, 4);
  elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R | SegmentFlags::PF_W);

  SegmentLayoutConstraint load;
  load.alignment = 4096;
  load.from_sec = text;
  load.to_sec = rodata;
  load.base_address = 0x1000;

  SECTION("Cannot have more constraints than segments") {
    std::vector<SegmentLayoutConstraint> constraints = {load, load};
    CHECK_THROWS_AS(calculate_layout(elf, &constraints), std::logic_error);
  }

  SECTION("Avoid access to out-of-bound section") {
    load.to_sec = 999;
    std::vector<SegmentLayoutConstraint> constraints = {load};
    CHECK_THROWS_AS(calculate_layout(elf, &constraints), std::logic_error);
  }

  SECTION("Avoid swapped [start, end]") {
    load.from_sec = rodata, load.to_sec = text;
    std::vector<SegmentLayoutConstraint> constraints = {load};
    CHECK_THROWS_AS(calculate_layout(elf, &constraints), std::logic_error);
  }

  SECTION("A segment less aligned than one of its sections") {
    // A loader may slide the segment by any multiple of p_align, so 4 cannot preserve .data's 16.
    elf.section_headers[data].sh_addralign = 16;
    load.alignment = 4;
    std::vector<SegmentLayoutConstraint> constraints = {load};
    CHECK_THROWS_AS(calculate_layout(elf, &constraints), std::logic_error);
  }

  SECTION("Two segments cannot partially overlap") {
    elf.add_segment(SegmentType::PT_NOTE, SegmentFlags::PF_R);
    load.to_sec = data;
    SegmentLayoutConstraint note = load;
    note.from_sec = data, note.to_sec = rodata;
    // .data belongs to both, so it would need an address from each of them.
    std::vector<SegmentLayoutConstraint> constraints = {load, note};
    CHECK_THROWS_AS(calculate_layout(elf, &constraints), std::logic_error);
  }
}
