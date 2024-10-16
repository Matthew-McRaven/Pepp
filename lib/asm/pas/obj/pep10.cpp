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
#include "asm/pas/ast/generic/attr_children.hpp"
#include "asm/pas/ast/generic/attr_sec.hpp"
#include "asm/pas/ast/generic/attr_symbol.hpp"
#include "asm/pas/operations/generic/combine.hpp"
#include "asm/pas/operations/pepp/bytes.hpp"
#include "asm/pas/operations/pepp/gather_ios.hpp"
#include "asm/symbol/table.hpp"
#include "asm/symbol/value.hpp"
#include "bits/operations/copy.hpp"
#include "isa/pep10.hpp"
#include "link/mmio.hpp"

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

static const auto strTabStr = ".strtab";

ELFIO::section *addStrTab(ELFIO::elfio &elf) {
  ELFIO::section *strTab = nullptr;
  for (auto &sec : elf.sections) {
    if (sec->get_name() == strTabStr && sec->get_type() == ELFIO::SHT_STRTAB) {
      strTab = sec.get();
      break;
    }
  }
  if (strTab == nullptr) {
    strTab = elf.sections.add(strTabStr);
    strTab->set_type(ELFIO::SHT_STRTAB);
  }
  return strTab;
}

QSharedPointer<ELFIO::elfio> pas::obj::pep10::createElf() {
  static const char p10mac[2] = {'p', 'x'};
  auto ret = QSharedPointer<ELFIO::elfio>::create();
  ret->create(ELFIO::ELFCLASS32, ELFIO::ELFDATA2MSB);
  ret->set_os_abi(ELFIO::ELFOSABI_NONE);
  ret->set_type(ELFIO::ET_EXEC);
  ret->set_machine(*(quint16 *)p10mac);
  // Create strtab/notes early, so that it will be before any code sections.
  addStrTab(*ret);
  ::obj::addMMIONoteSection(*ret);
  return ret;
}

void writeSymtab(ELFIO::elfio &elf, symbol::Table &table, QString prefix) {
  using namespace Qt::StringLiterals;
  auto strTab = addStrTab(elf);

  auto symTab = elf.sections.add(u"%1.symtab"_s.arg(prefix).toStdString());
  symTab->set_type(ELFIO::SHT_SYMTAB);
  symTab->set_info(0);
  symTab->set_addr_align(2);
  symTab->set_entry_size(elf.get_default_entry_size(ELFIO::SHT_SYMTAB));
  symTab->set_link(strTab->get_index());

  // Attempt to pool strings when possible, to reduce final binary size.
  // Probably O(n^2), but n should be small for Pep/10.
  ELFIO::string_section_accessor strAc(strTab);
  auto findOrCreateStr = [&](const std::string &str) {
    auto tabStart = strTab->get_data();
    auto tabEnd = tabStart + strTab->get_size();
    auto iter = std::search(tabStart, tabEnd, str.cbegin(), str.cend());
    if (iter != tabEnd) return (ELFIO::Elf_Word)(iter - tabStart);
    return strAc.add_string(str.data());
  };

  ELFIO::symbol_section_accessor symAc(elf, symTab);
  for (auto [name, entry] : table.entries()) {
    auto nameIdx = findOrCreateStr(name.toStdString());
    auto secIdx = entry->section_index;
    auto value = entry->value;

    quint8 type = ELFIO::STT_NOTYPE;
    if (value->type() == symbol::Type::kCode) type = ELFIO::STT_FUNC;
    else if (value->type() == symbol::Type::kObject) type = ELFIO::STT_OBJECT;
    else if (value->type() == symbol::Type::kConstant) {
      type = ELFIO::STT_OBJECT;
      secIdx = ELFIO::SHN_ABS;
    }

    quint8 bind = ELFIO::STB_LOCAL;
    if (entry->binding == symbol::Binding::kGlobal) bind = ELFIO::STB_GLOBAL;
    else if (entry->binding == symbol::Binding::kImported) bind = ELFIO::STB_WEAK;

    quint8 info = (bind << 4) + (type & 0xf);

    if (entry->state == symbol::DefinitionState::kUndefined) secIdx = ELFIO::SHN_UNDEF;
    symAc.add_symbol(nameIdx, value->value()(), entry->value->size(), info, 0,
                     secIdx); // leave other as 0, don't mess with visibility.
  }

  // To be elf compliant, local symbols must be before all other kinds.
  symAc.arrange_local_symbols();
}

void writeTree(ELFIO::elfio &elf, pas::ast::Node &node, QString prefix, bool isOS) {
  using namespace pas;
  using namespace Qt::StringLiterals;
  ELFIO::segment *activeSeg = nullptr;

  auto getOrCreateBSS = [&](ast::generic::SectionFlags::Flags &flags) {
    if (activeSeg == nullptr || activeSeg->get_file_size() != 0) {
      activeSeg = elf.segments.add();
      activeSeg->set_type(ELFIO::PT_LOAD);
      ELFIO::Elf_Word elfFlags = 0;
      elfFlags |= flags.R ? ELFIO::PF_R : 0;
      elfFlags |= flags.W ? ELFIO::PF_W : 0;
      elfFlags |= flags.X ? ELFIO::PF_X : 0;
      activeSeg->set_flags(elfFlags);
      activeSeg->set_physical_address(-1);
      activeSeg->set_virtual_address(-1);
    }
    return activeSeg;
  };

  auto getOrCreateBits = [&](ast::generic::SectionFlags::Flags &flags) {
    if (activeSeg == nullptr || activeSeg->get_file_size() == 0 ||
        !(((activeSeg->get_flags() & ELFIO::PF_R) > 0 == flags.R) &&
          ((activeSeg->get_flags() & ELFIO::PF_W) > 0 == flags.W) &&
          ((activeSeg->get_flags() & ELFIO::PF_X) > 0 == flags.X))) {
      activeSeg = elf.segments.add();
      activeSeg->set_type(isOS ? ELFIO::PT_LOAD : ELFIO::PT_LOPROC + 1);
      ELFIO::Elf_Word elfFlags = 0;
      elfFlags |= flags.R ? ELFIO::PF_R : 0;
      elfFlags |= flags.W ? ELFIO::PF_W : 0;
      elfFlags |= flags.X ? ELFIO::PF_X : 0;
      activeSeg->set_flags(elfFlags);
      activeSeg->set_physical_address(-1);
      activeSeg->set_virtual_address(-1);
    }
    return activeSeg;
  };

  for (auto &astSec : ast::children(node)) {
    // Strip leading . from sections, since we will be adding prefix+".".
    auto secName = astSec->get<ast::generic::SectionName>().value;
    if (secName.startsWith(".")) secName = secName.mid(1);
    auto secFlags = astSec->get<ast::generic::SectionFlags>().value;
    auto traits = pas::ops::generic::detail::getTraits(*astSec);
    auto align = traits.alignment;
    ELFIO::Elf64_Addr baseAddr = traits.base;
    auto size = traits.size;
    auto bytes = pas::ops::pepp::toBytes<isa::Pep10>(*astSec);
    if (size == 0) continue; // 0-sized sections are meaningless, do not emit.

    auto sec = elf.sections.add(u"%1.%2"_s.arg(prefix, secName).toStdString());
    // All sections from AST correspond to bits in Pep/10 memory, so alloc
    auto shFlags = ELFIO::SHF_ALLOC;
    shFlags |= secFlags.X ? ELFIO::SHF_EXECINSTR : 0;
    shFlags |= secFlags.W ? ELFIO::SHF_WRITE : 0;
    sec->set_flags(shFlags);
    sec->set_addr_align(align);

    if (secFlags.Z) {
      sec->set_type(ELFIO::SHT_NOBITS);
      sec->set_size(size);
    } else {
      Q_ASSERT(bytes.size() == size);
      sec->set_type(ELFIO::SHT_PROGBITS);
      sec->set_data((const char *)bytes.constData(), size);
    }

    // If we are before the first segment of the OS, add the user address space.
    if (isOS && activeSeg == nullptr) {
      auto userSeg = elf.segments.add();
      userSeg->set_align(1);
      userSeg->set_file_size(0);
      userSeg->set_virtual_address(0x0);
      userSeg->set_physical_address(0x0);
      userSeg->set_memory_size(baseAddr);
      userSeg->set_type(ELFIO::PT_LOAD);
      userSeg->set_flags(ELFIO::PF_R | ELFIO::PF_W | ELFIO::PF_X);
    }

    // Both implicitly capture isOS.
    if (secFlags.Z) getOrCreateBSS(secFlags);
    else getOrCreateBits(secFlags);

    activeSeg->add_section(sec, align);
    activeSeg->set_physical_address(std::min(activeSeg->get_physical_address(), baseAddr));
    activeSeg->set_virtual_address(std::min(activeSeg->get_virtual_address(), baseAddr));

    // Field not re-computed on its own. Failure to compute will cause readelf
    // to crash.
    // TODO: in the future, handle alignment correctly?
    activeSeg->set_memory_size(activeSeg->get_memory_size() + size);

    // Update the section index of all symbols in this section, otherwise symtab
    // will link against SHN_UNDEF.
    for (auto &line : ast::children(*astSec))
      if (line->has<ast::generic::SymbolDeclaration>())
        line->get<ast::generic::SymbolDeclaration>().value->section_index = sec->get_index();
  }

  if (node.has<ast::generic::SymbolTable>()) writeSymtab(elf, *node.get<ast::generic::SymbolTable>().value, prefix);
}

void pas::obj::pep10::writeOS(ELFIO::elfio &elf, ast::Node &os) {
  writeTree(elf, os, "os", true);
  elf.set_entry(/*TODO:determine OS entry point*/ 0x000);

  auto mmios = pas::ops::pepp::gatherIODefinitions(os);

  // Find symbol table for os or crash.
  ELFIO::section *symTab = nullptr;
  for (auto &sec : elf.sections) {
    if (sec->get_type() == ELFIO::SHT_SYMTAB && sec->get_name() == "os.symtab") symTab = &*sec;
  }
  Q_ASSERT(symTab != nullptr);
  ::obj::addMMIODeclarations(elf, symTab, mmios);
  ::obj::setBootFlagAddress(elf);
}

void pas::obj::pep10::writeUser(ELFIO::elfio &elf, ast::Node &user) {
  writeTree(elf, user, "usr", false);

  // Add notes regarding MMIO buffering.
  for (auto &seg : elf.segments) {
    // Only LOPROC+1 segments need buffering.
    if (seg->get_type() != ELFIO::PT_LOPROC + 0x1) continue;
    ::obj::addMMIBuffer(elf, seg.get());
    // The "buffered" segments need to not overlap with default RWX segment,
    // otherwise user program will always be loaded automatically.
    // So, adjust the addreses+sizes of the default segment to exclude our
    // buffered one.
    auto newAddr = seg->get_physical_address() + seg->get_memory_size();
    auto delta = newAddr - elf.segments[0]->get_physical_address();
    elf.segments[0]->set_physical_address(newAddr);
    elf.segments[0]->set_virtual_address(newAddr);
    elf.segments[0]->set_memory_size(elf.segments[0]->get_memory_size() - delta);
  }
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
  auto seg = elf.segments.add();
  seg->set_align(1);
  seg->set_virtual_address(0x0);
  seg->set_physical_address(0x0);
  seg->set_memory_size(size);
  seg->set_type(ELFIO::PT_LOPROC + 1);
  seg->set_flags(ELFIO::PF_R | ELFIO::PF_W | ELFIO::PF_X);
  seg->add_section(sec, 1);

  // Add notes regarding MMIO buffering.
  for (auto &seg : elf.segments) {
    // Only LOPROC+1 segments need buffering.
    if (seg->get_type() != ELFIO::PT_LOPROC + 0x1) continue;
    ::obj::addMMIBuffer(elf, seg.get());
    // The "buffered" segments need to not overlap with default RWX segment,
    // otherwise user program will always be loaded automatically.
    // So, adjust the addreses+sizes of the default segment to exclude our
    // buffered one.
    auto newAddr = seg->get_physical_address() + seg->get_memory_size();
    auto delta = newAddr - elf.segments[0]->get_physical_address();
    elf.segments[0]->set_physical_address(newAddr);
    elf.segments[0]->set_virtual_address(newAddr);
    elf.segments[0]->set_memory_size(elf.segments[0]->get_memory_size() - delta);
  }
}
