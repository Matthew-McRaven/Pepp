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
#include "core/libs/elf/packed_access_array.hpp"
#include "core/libs/elf/packed_access_note.hpp"
#include "core/libs/elf/packed_elf.hpp"
#include "core/libs/elf/packed_ops.hpp"
#include "core/libs/elf/packed_types.hpp"

namespace {
bool write(const std::string &fname, const std::span<const u8> &data) {
  std::ofstream out(fname, std::ios::binary);
  if (!out.is_open()) return false;
  out.write(reinterpret_cast<const char *>(data.data()), data.size());
  return out.good();
}
} // namespace

TEST_CASE("Test combining sections, 32-bit", "[scope:elf][kind:unit][arch:*]") {
  using namespace pepp::bts;
  using Packed = PackedGrowableElfLE32;

  SECTION("Write pepp.mmios notes") {
    Packed elf_1(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP8, ElfABI::ELFOSABI_NONE);
    Packed elf_2(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP8, ElfABI::ELFOSABI_NONE);
    u16 note_1_idx = 0, note_2_idx = 0;
    {
      ensure_section_header_table(elf_1);
      add_named_section(elf_1, "wasted", SectionTypes::SHT_NOBITS);
      note_1_idx = add_named_section(elf_1, ".note", SectionTypes::SHT_NOTE);
      PackedNoteWriter<ElfBits::b32, ElfEndian::le> note_writer(elf_1, note_1_idx);
      char note_data[6]{0x00, 0x07, 0x00, 0x00, 0x01, 0x13};
      note_writer.add_note(std::span<const char>{"pepp.mmios"}, std::span<const char>{note_data, 6}, 0x12);
      note_data[5] = 0x16;
      note_writer.add_note(std::span<const char>{"pepp.mmios"}, std::span<const char>{note_data, 6}, 0x11);
    }
    {
      ensure_section_header_table(elf_2);
      add_named_section(elf_2, "more", SectionTypes::SHT_NOBITS);
      add_named_section(elf_2, "waste", SectionTypes::SHT_NOBITS);
      note_2_idx = add_named_section(elf_2, ".note", SectionTypes::SHT_NOTE);
      PackedNoteWriter<ElfBits::b32, ElfEndian::le> note_writer(elf_2, note_2_idx);
      char note_data[6]{0x00, 0x07, 0x00, 0x00, 0x01, 0x1a};
      note_writer.add_note(std::span<const char>{"pepp.mmios"}, std::span<const char>{note_data, 6}, 0x12);
      note_data[5] = 0x19;
      note_writer.add_note(std::span<const char>{"pepp.mmios"}, std::span<const char>{note_data, 6}, 0x13);
    }
    Packed elf_3(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP8, ElfABI::ELFOSABI_NONE);
    ensure_section_header_table(elf_3);
    auto note_idx = add_named_section(elf_3, ".note", SectionTypes::SHT_NOTE);
    auto comb = std::make_shared<CombiningStorage>();
    elf_3.section_data[note_idx] = comb;
    comb->add_section(&elf_1, note_1_idx);
    comb->add_section(&elf_2, note_2_idx);
    auto layout = calculate_layout(elf_3);
    std::vector<u8> data(size_for_layout(layout), 0);
    write(data, layout);
    write("combined_notes.elf", data);

    PackedInputElfFile<ElfBits::b32, ElfEndian::le> in("combined_notes.elf");
    // As measured from ehdr_notes.elf
    CHECK(in.section_headers[note_idx].sh_size == 0x80);
  }
}
