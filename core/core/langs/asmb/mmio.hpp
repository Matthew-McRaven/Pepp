/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
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

#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "core/formats/elf/packed_access_note.hpp"
#include "core/formats/elf/packed_access_symbol.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/math/bitmanip/strings.hpp"

namespace pepp::tc {
// A named memory-mapped IO port.
struct IO {
  std::string name;
  enum class Type { kInput, kOutput, kIDE } type;
};

// Assign an inclusive address range to a port.
struct AddressedIO : public IO {
  u16 minOffset, maxOffset;
};

// MMIO ports are declared by notes in a non-standard SHT_NOTE section, which the old simulator's loader reads
// (obj::getMMIODeclarations). This whole file is removable once the new simulator is in use.
static constexpr const char *mmioNoteSection = ".mmio";
static constexpr std::string_view mmioNoteOwner = "pepp.mmios";
enum class MMIONoteType : u32 { Input = 0x11, Output = 0x12, IDE = 0x13 };

#pragma pack(push, 1)
// A note's descriptor names the port's symbol. Big-endian regardless of the file's byte order.
struct PackedMMIODescriptor {
  ub16 symtab;
  ub32 symbol;
};
#pragma pack(pop)
static_assert(sizeof(PackedMMIODescriptor) == 6);

// Adds the MMIO note section to `elf`, naming ports by the symbols of section `symtab`.
template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E> class MMIONoteWriter {
public:
  MMIONoteWriter(pepp::bts::PackedGrowableElfFile<B, E> &elf, u16 symtab_shndx);
  // False if no symbol is named io.name.
  bool add(const IO &io);

private:
  pepp::bts::PackedGrowableElfFile<B, E> &_elf;
  u16 _symtab, _note;
};

// Reads the MMIO note section of `elf`, if any.
template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E> class MMIONoteReader {
public:
  explicit MMIONoteReader(const pepp::bts::PackedElf<B, E> &elf);
  // Ports whose note names a valid symbol, in note order.
  std::vector<AddressedIO> declarations() const;

private:
  const pepp::bts::PackedElf<B, E> &_elf;
  u16 _note;
};

template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E>
MMIONoteWriter<B, E>::MMIONoteWriter(pepp::bts::PackedGrowableElfFile<B, E> &elf, u16 symtab_shndx)
    : _elf(elf), _symtab(symtab_shndx),
      _note(pepp::bts::add_named_section(elf, mmioNoteSection, pepp::bts::SectionTypes::SHT_NOTE)) {
  _elf.section_headers[_note].sh_addralign = 4;
}

template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E> bool MMIONoteWriter<B, E>::add(const IO &io) {
  pepp::bts::PackedSymbolReader<B, E> symbols(_elf, _symtab);
  u32 symbol = 1;
  while (symbol < symbols.symbol_count() && bits::until_nul(symbols.get_symbol_name(symbol)) != io.name) ++symbol;
  if (symbol >= symbols.symbol_count()) return false;
  MMIONoteType type = MMIONoteType::Input;
  switch (io.type) {
  case IO::Type::kInput: type = MMIONoteType::Input; break;
  case IO::Type::kOutput: type = MMIONoteType::Output; break;
  case IO::Type::kIDE: type = MMIONoteType::IDE; break;
  }
  const PackedMMIODescriptor desc{.symtab = _symtab, .symbol = symbol};
  pepp::bts::PackedNoteWriter<B, E>(_elf, _note)
      .add_note(mmioNoteOwner, std::string_view(reinterpret_cast<const char *>(&desc), sizeof(desc)),
                bits::to_underlying(type));
  return true;
}

template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E>
MMIONoteReader<B, E>::MMIONoteReader(const pepp::bts::PackedElf<B, E> &elf)
    : _elf(elf), _note(pepp::bts::find_named_section(elf, mmioNoteSection)) {}

template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E>
std::vector<AddressedIO> MMIONoteReader<B, E>::declarations() const {
  std::vector<AddressedIO> ret;
  if (_note == 0) return ret;
  pepp::bts::PackedNoteReader<B, E> notes(_elf, _note);
  for (u32 it = 0; it < notes.note_count(); ++it) {
    const auto note = notes.get_note(it);
    if (!note || note->desc.size() != sizeof(PackedMMIODescriptor)) continue;
    else if (bits::until_nul(note->name) != mmioNoteOwner) continue;
    const auto *desc = reinterpret_cast<const PackedMMIODescriptor *>(note->desc.data());
    const u16 symtab = desc->symtab;
    const u32 index = desc->symbol;
    if (symtab == 0 || symtab >= _elf.section_headers.size()) continue;
    pepp::bts::PackedSymbolReader<B, E> symbols(_elf, symtab);
    if (index == 0 || index >= symbols.symbol_count()) continue;
    const auto symbol = symbols.get_symbol(index);
    const u16 value = static_cast<u16>(symbol.st_value), size = static_cast<u16>(symbol.st_size);
    AddressedIO io{{.name = std::string(bits::until_nul(symbols.get_symbol_name(index))), .type = IO::Type::kInput},
                   value, value};
    switch (static_cast<MMIONoteType>(static_cast<u32>(note->type))) {
    case MMIONoteType::Input: io.type = IO::Type::kInput, io.maxOffset = value + (size ? size - 1 : 0); break;
    case MMIONoteType::Output: io.type = IO::Type::kOutput, io.maxOffset = value + (size ? size - 1 : 0); break;
    // An IDE controller has 8 bytes of registers.
    case MMIONoteType::IDE: io.type = IO::Type::kIDE, io.maxOffset = value + 7; break;
    default: continue;
    }
    ret.push_back(std::move(io));
  }
  return ret;
}
} // namespace pepp::tc
