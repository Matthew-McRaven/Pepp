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

#include "./pep9.hpp"
#include "core/isa/pep/pep9.hpp"
#include "./common.hpp"
#include "core/bitmanip/copy.hpp"
#include "toolchain/link/mmio.hpp"
#include "toolchain/pas/ast/generic/attr_children.hpp"
#include "toolchain/pas/operations/pepp/gather_ios.hpp"

namespace {
void writeTree(ELFIO::elfio &elf, pas::ast::Node &node, QString prefix, bool isOS) {
  using namespace pas;
  using namespace Qt::StringLiterals;

  static const quint16 stackbase = 0xFB8F;
  static const quint16 textBase = 0xFC17;

  if (!isOS) {
    auto bytes = pas::ops::pepp::toBytes<isa::Pep9>(node);
    auto sec = elf.sections.add(u"%1.%2"_s.arg(prefix, "txt").toStdString());
    sec->set_flags(ELFIO::SHF_ALLOC | ELFIO::SHF_EXECINSTR | ELFIO::SHF_WRITE);
    sec->set_addr_align(1);
    sec->set_type(ELFIO::SHT_PROGBITS);
    sec->set_data((const char *)bytes.constData(), bytes.size());
    auto seg = elf.segments[0];
    seg->add_section(sec, 1);
    // Update the section index of all symbols in this section, otherwise symtab
    // will link against SHN_UNDEF.
    for (auto &line : ast::children(node))
      if (line->has<ast::generic::SymbolDeclaration>())
        line->get<ast::generic::SymbolDeclaration>().value->section_index = sec->get_index();

  } else {
    // As read from the source code of Pep/9 OS.
    auto lastByteBeforeBurn = 136;
    auto stack = elf.sections.add(u"%1.%2"_s.arg(prefix, "stack").toStdString());
    stack->set_flags(ELFIO::SHF_ALLOC | ELFIO::SHF_WRITE);
    stack->set_addr_align(1);
    stack->set_type(ELFIO::SHT_NOBITS);
    stack->set_size(lastByteBeforeBurn);

    auto text = elf.sections.add(u"%1.%2"_s.arg(prefix, "txt").toStdString());
    auto text_bytes = pas::ops::pepp::toBytes<isa::Pep9>(node);
    text->set_flags(ELFIO::SHF_ALLOC | ELFIO::SHF_EXECINSTR);
    text->set_addr_align(1);
    text->set_type(ELFIO::SHT_PROGBITS);
    text->set_data((const char *)text_bytes.constData() + lastByteBeforeBurn, text_bytes.size() - lastByteBeforeBurn);

    // Create stack segment
    auto userSeg = elf.segments.add();
    userSeg->set_align(1);
    userSeg->set_file_size(0);
    userSeg->set_virtual_address(0x0);
    userSeg->set_physical_address(0x0);
    userSeg->set_memory_size(stackbase);
    userSeg->set_type(ELFIO::PT_LOAD);
    userSeg->set_flags(ELFIO::PF_R | ELFIO::PF_W | ELFIO::PF_X);

    // Create stack segment
    auto stack_seg = elf.segments.add();
    stack_seg->set_type(ELFIO::PT_LOAD);
    stack_seg->set_flags(ELFIO::PF_R | ELFIO::PF_W);
    stack_seg->set_physical_address(stackbase);
    stack_seg->set_virtual_address(stackbase);
    stack_seg->add_section(stack, 1);
    stack_seg->set_memory_size(lastByteBeforeBurn);

    auto text_seg = elf.segments.add();
    text_seg->set_type(ELFIO::PT_LOAD);
    text_seg->set_flags(ELFIO::PF_R | ELFIO::PF_X);
    text_seg->set_physical_address(textBase);
    text_seg->set_virtual_address(textBase);
    text_seg->add_section(text, 1);
    text_seg->set_memory_size(text_bytes.size() - lastByteBeforeBurn);

    // repeat for stack & text sections.
    // Update the section index of all symbols in this section, otherwise symtab
    // will link against SHN_UNDEF.
    bool seenBurn = false;
    for (auto &line : ast::children(node)) {
      if (line->has<ast::generic::Directive>() && line->get<ast::generic::Directive>().value == "BURN") seenBurn = true;
      if (line->has<ast::generic::SymbolDeclaration>()) {
        if (!seenBurn) line->get<ast::generic::SymbolDeclaration>().value->section_index = stack->get_index();
        else line->get<ast::generic::SymbolDeclaration>().value->section_index = text->get_index();
      }
    }
  }
  if (node.has<ast::generic::SymbolTable>())
    pas::obj::common::writeSymtab(elf, *node.get<ast::generic::SymbolTable>().value, prefix);
}
} // namespace

QSharedPointer<ELFIO::elfio> pas::obj::pep9::createElf() {
  static const char p9mac[2] = {'p', '9'};
  quint16 mac;
  bits::memcpy_endian({(quint8 *)&mac, 2}, bits::hostOrder(), {(const quint8 *)p9mac, 2}, bits::Order::BigEndian);

  auto ret = QSharedPointer<ELFIO::elfio>::create();
  ret->create(ELFIO::ELFCLASS32, ELFIO::ELFDATA2MSB);
  ret->set_os_abi(ELFIO::ELFOSABI_NONE);
  ret->set_type(ELFIO::ET_EXEC);
  ret->set_machine(mac);
  // Create strtab/notes early, so that it will be before any code sections.
  common::addStrTab(*ret);
  ::obj::addMMIONoteSection(*ret);
  return ret;
}

void pas::obj::pep9::writeOS(ELFIO::elfio &elf, ast::Node &os) {
  writeTree(elf, os, "os", true);

  elf.set_entry(/*TODO:determine OS entry point*/ 0x000);

  // Manually gather MMIOs  (charIn / charOut).
  QList<::obj::IO> mmios = {{.name = "charOut", .type = ::obj::IO::Type::kOutput},
                            {.name = "charIn", .type = ::obj::IO::Type::kInput}};

  // Find symbol table for os or crash.
  ELFIO::section *symTab = nullptr;
  for (auto &sec : elf.sections) {
    if (sec->get_type() == ELFIO::SHT_SYMTAB && sec->get_name() == "os.symtab") symTab = &*sec;
  }
  Q_ASSERT(symTab != nullptr);

  ::obj::addMMIODeclarations(elf, symTab, mmios);
}

void pas::obj::pep9::writeUser(ELFIO::elfio &elf, ast::Node &user) {
  writeTree(elf, user, "usr", false);
}

void pas::obj::pep9::writeUser(ELFIO::elfio &elf, const std::vector<u8> &bytes) {
  auto align = 1;
  ELFIO::Elf64_Addr size = bytes.size();
  auto sec = elf.sections.add("usr.txt");
  sec->set_type(ELFIO::SHT_PROGBITS);
  // All sections from AST correspond to bits in Pep/9 memory, so alloc
  sec->set_flags(ELFIO::SHF_ALLOC | ELFIO::SHF_WRITE | ELFIO::SHF_EXECINSTR);
  sec->set_addr_align(align);
  sec->set_data((const char *)bytes.data(), size);
  auto seg = elf.segments[0];
  seg->add_section(sec, 1);
}
