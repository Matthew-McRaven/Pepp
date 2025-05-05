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

#include "./pep10.hpp"
#include "./common.hpp"
#include "asm/pas/ast/generic/attr_children.hpp"
#include "asm/pas/ast/generic/attr_sec.hpp"
#include "asm/pas/operations/pepp/gather_ios.hpp"
#include "isa/pep10.hpp"
#include "link/mmio.hpp"
#include "utils/bits/copy.hpp"

void pas::obj::pep10::combineSections(ast::Node &root) {
  QList<QSharedPointer<pas::ast::Node>> newChildren{};
  QMap<QString, QSharedPointer<ast::Node>> newChildrenMap = {};
  auto oldChildren = ast::children(root);

  // Iterate over all sections (that may have duplicate names), and combine the
  // sub-nodes of sections with duplicate names. newChildren preserves the
  // relative order of the first appearance of each section.
  for (auto &oldChild : oldChildren) {
    auto childName = oldChild->get<pas::ast::generic::SectionName>().value;
    if (auto newSec = newChildrenMap.find(childName); newSec == newChildrenMap.end()) {
      newChildrenMap[childName] = oldChild;
      newChildren.push_back(oldChild);
    } else {
      auto newChild = newChildrenMap[childName];
      auto newSubChildren = ast::children(*newChild);
      auto oldSubChildren = ast::children(*oldChild);
      for (auto &oldSubChild : oldSubChildren) ast::setParent(*oldSubChild, newChild);
      newSubChildren.append(oldSubChildren);

      newChild->set(ast::generic::Children{.value = newSubChildren});
    }
  }

  root.set(ast::generic::Children{.value = newChildren});
}

QSharedPointer<ELFIO::elfio> pas::obj::pep10::createElf() {
  static const char p10mac[2] = {'p', 'x'};
  quint16 mac;
  bits::memcpy_endian({(quint8 *)&mac, 2}, bits::hostOrder(), {(const quint8 *)p10mac, 2}, bits::Order::BigEndian);
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

void pas::obj::pep10::writeOS(ELFIO::elfio &elf, ast::Node &os) {
  common::writeTree<isa::Pep10>(elf, os, "os", true);
  elf.set_entry(/*TODO:determine OS entry point*/ 0x000);

  auto mmios = pas::ops::pepp::gatherIODefinitions(os);

  // Find symbol table for os or crash.
  ELFIO::section *symTab = nullptr;
  for (auto &sec : elf.sections) {
    if (sec->get_type() == ELFIO::SHT_SYMTAB && sec->get_name() == "os.symtab") symTab = &*sec;
  }
  Q_ASSERT(symTab != nullptr);
  ::obj::addMMIODeclarations(elf, symTab, mmios);
}

void pas::obj::pep10::writeUser(ELFIO::elfio &elf, ast::Node &user) {
  common::writeTree<isa::Pep10>(elf, user, "usr", false);
}

void pas::obj::pep10::writeUser(ELFIO::elfio &elf, QList<quint8> bytes) {
  auto align = 1;
  ELFIO::Elf64_Addr baseAddr = 0;
  auto size = bytes.size();
  auto sec = elf.sections.add("usr.txt");
  sec->set_type(ELFIO::SHT_PROGBITS);
  // All sections from AST correspond to bits in Pep/10 memory, so alloc
  sec->set_flags(ELFIO::SHF_ALLOC | ELFIO::SHF_WRITE | ELFIO::SHF_EXECINSTR);
  sec->set_addr_align(align);
  sec->set_data((const char *)bytes.constData(), size);
  auto seg = elf.segments[0];
  seg->add_section(sec, 1);
}
