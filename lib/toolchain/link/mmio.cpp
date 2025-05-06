/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
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

#include "./mmio.hpp"
#include "utils/bits/copy.hpp"
const ELFIO::section *obj::getMMIONoteSection(const ELFIO::elfio &elf) {
  for (auto &sec : elf.sections)
    if (sec->get_type() == ELFIO::SHT_NOTE) return &*sec;
  return nullptr;
}

ELFIO::section *obj::addMMIONoteSection(ELFIO::elfio &elf) {
  // Only create note sec & segment if they do not already exist.
  ELFIO::section *noteSec = nullptr;
  for (auto &sec : elf.sections) {
    if (sec->get_type() == ELFIO::SHT_NOTE) noteSec = &*sec;
  }
  if (noteSec == nullptr) {
    noteSec = elf.sections.add(".mmio");
    noteSec->set_type(ELFIO::SHT_NOTE);
  }

  return noteSec;
}

void addNoteSeg(ELFIO::elfio &elf) {
  auto noteSec = obj::addMMIONoteSection(elf);
  ELFIO::segment *noteSeg = nullptr;
  for (auto &seg : elf.segments) {
    if (seg->get_type() == ELFIO::PT_NOTE) noteSeg = &*seg;
  }
  if (noteSeg == nullptr) {
    noteSeg = elf.segments.add();
    noteSeg->set_type(ELFIO::PT_NOTE);
    noteSeg->add_section(noteSec, 1);
  }
}

void obj::addMMIODeclarations(ELFIO::elfio &elf, ELFIO::section *symTab, QList<IO> mmios) {
  ELFIO::symbol_section_accessor symTabAc(elf, symTab);
  auto noteSec = addMMIONoteSection(elf);
  addNoteSeg(elf);
  auto noteAc = ELFIO::note_section_accessor(elf, noteSec);
  // Iterate over the symtab first, since it is much longer than MMIO list
  for (ELFIO::Elf_Xword it = 1; it < symTabAc.get_symbols_num(); it++) {
    std::string name;
    ELFIO::Elf64_Addr value;
    ELFIO::Elf_Xword size;
    unsigned char bind, type, other;
    ELFIO::Elf_Half index;
    // Skip to next iteration if it does not name a valid symbol
    if (!symTabAc.get_symbol(it, name, value, size, bind, type, index, other)) continue;

    // Search the MMIO list for a matching entry, skip to next iteration if no
    // match.
    QString nameQs = QString::fromStdString(name);
    auto target =
        std::find_if(mmios.cbegin(), mmios.cend(), [&nameQs](const ::obj::IO &io) { return io.name == nameQs; });
    if (target == mmios.cend()) continue;

    // Must use copy helper to maintain stable bit order between host
    // platforms.
    quint8 desc[2 + 4]; // ELF_half (16b for symtab section index) and
                        // ELF32_WORD (32b for symbol index)
    bits::span descSpan = {desc};
    auto stIndex = symTab->get_index();
    bits::memcpy_endian(descSpan.first(2), bits::Order::BigEndian, stIndex);
    bits::memcpy_endian(descSpan.subspan(2), bits::Order::BigEndian, it);
    switch (target->type) {
    case ::obj::IO::Type::kInput: noteAc.add_note(0x11, "pepp.mmios", (char *)desc, sizeof(desc)); break;
    case ::obj::IO::Type::kOutput: noteAc.add_note(0x12, "pepp.mmios", (char *)desc, sizeof(desc)); break;
    case ::obj::IO::Type::kIDE: noteAc.add_note(0x13, "pepp.mmios", (char *)desc, sizeof(desc)); break;
    }
  }
}

void obj::addIDEDeclaration(ELFIO::elfio &elf, ELFIO::section *symTab, QString symbol) {
  ELFIO::symbol_section_accessor symTabAc(elf, symTab);
  auto noteSec = addMMIONoteSection(elf);
  addNoteSeg(elf);
  auto noteAc = ELFIO::note_section_accessor(elf, noteSec);
  // Iterate over the symtab first, since it is much longer than MMIO list
  for (ELFIO::Elf_Xword it = 1; it < symTabAc.get_symbols_num(); it++) {
    std::string name;
    ELFIO::Elf64_Addr value;
    ELFIO::Elf_Xword size;
    unsigned char bind, type, other;
    ELFIO::Elf_Half index;
    // Skip to next iteration if it does not name a valid symbol
    if (!symTabAc.get_symbol(it, name, value, size, bind, type, index, other)) continue;

    QString nameQs = QString::fromStdString(name);
    if (symbol != nameQs) continue;

    // Must use copy helper to maintain stable bit order between host
    // platforms.
    quint8 desc[2 + 4]; // ELF_half (16b for symtab section index) and
                        // ELF32_WORD (32b for symbol index)
    bits::span descSpan = {desc};
    auto stIndex = symTab->get_index();
    bits::memcpy_endian(descSpan.first(2), bits::Order::BigEndian, stIndex);
    bits::memcpy_endian(descSpan.subspan(2), bits::Order::BigEndian, it);
    noteAc.add_note(0x13, "pepp.mmios", (char *)desc, sizeof(desc));
  }
}

QList<obj::AddressedIO> obj::getMMIODeclarations(const ELFIO::elfio &elf) {
  auto noteSec = getMMIONoteSection(elf);
  if (noteSec == nullptr) return {};
  auto noteAc = ELFIO::const_note_section_accessor(elf, noteSec);
  auto ret = QList<obj::AddressedIO>{};
  ELFIO::Elf_Word noteType = 0, descSize = 0;
  std::string name;
  char *desc = 0;
  for (int it = 0; it < noteAc.get_notes_num(); it++) {
    // Check that note exists and is from me.
    if (!noteAc.get_note(it, noteType, name, desc, descSize)) continue;
    else if (name != "pepp.mmios") continue;
    auto descSpan = bits::span<const quint8>{reinterpret_cast<const quint8 *>(desc), descSize};
    // Copy out symbol table index + symbol index into that table.
    auto stIndex = bits::memcpy_endian<ELFIO::Elf_Half>(descSpan.first(2), bits::Order::BigEndian);
    auto symIt = bits::memcpy_endian<ELFIO::Elf_Xword>(descSpan.subspan(2), bits::Order::BigEndian);
    auto symTab = elf.sections[stIndex];
    auto symTabAc = ELFIO::symbol_section_accessor(elf, symTab);
    // If symbol exists, extract contents.
    ELFIO::Elf64_Addr value;
    ELFIO::Elf_Xword size;
    unsigned char bind, symbolType, other;
    ELFIO::Elf_Half index;
    // Skip to next iteration if it does not name a valid symbol.
    if (!symTabAc.get_symbol(symIt, name, value, size, bind, symbolType, index, other)) continue;
    IO::Type type;
    switch (noteType) {
    case 0x11: type = IO::Type::kInput; break;
    case 0x12: type = IO::Type::kOutput; break;
    case 0x13: type = IO::Type::kIDE; break;
    }
    if (noteType == 0x11 || noteType == 0x12) {
      ret.push_back(obj::AddressedIO{{.name = QString::fromStdString(name), .type = type},
                                     static_cast<quint16>(value),
                                     static_cast<quint16>(value + std::max<decltype(size)>(size - 1, 0))});
    } else if (noteType == 0x13) {
      ret.push_back(obj::AddressedIO{{.name = QString::fromStdString(name), .type = type},
                                     static_cast<quint16>(value),
                                     // IDE uses 8 bytes worth of registers.
                                     static_cast<quint16>(value + 7)});
    }
  }
  return ret;
}
