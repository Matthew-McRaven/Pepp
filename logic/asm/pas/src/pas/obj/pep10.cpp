#include "./pep10.hpp"
#include "pas/ast/generic/attr_children.hpp"
#include "pas/ast/generic/attr_sec.hpp"
#include "pas/ast/generic/attr_symbol.hpp"
#include "pas/isa/pep10.hpp"
#include "pas/operations/generic/combine.hpp"
#include "pas/operations/pepp/bytes.hpp"
#include "symbol/table.hpp"
#include "symbol/value.hpp"

void pas::obj::pep10::combineSections(ast::Node &root) {
  QList<QSharedPointer<pas::ast::Node>> newChildren{};
  QMap<QString, QSharedPointer<ast::Node>> newChildrenMap = {};
  auto oldChildren = ast::children(root);

  // Iterate over all sections (that may have duplicate names), and combine the
  // sub-nodes of sections with duplicate names. newChildren preserves the
  // relative order of the first appearance of each section.
  for (auto &oldChild : oldChildren) {
    auto childName = oldChild->get<pas::ast::generic::SectionName>().value;
    if (auto newSec = newChildrenMap.find(childName);
        newSec == newChildrenMap.end()) {
      newChildrenMap[childName] = oldChild;
      newChildren.push_back(oldChild);
    } else {
      auto newChild = newChildrenMap[childName];
      auto newSubChildren = ast::children(*newChild);
      auto oldSubChildren = ast::children(*oldChild);
      for (auto &oldSubChild : oldSubChildren)
        ast::setParent(*oldSubChild, newChild);
      newSubChildren.append(oldSubChildren);

      newChild->set(ast::generic::Children{.value = newSubChildren});
    }
  }

  root.set(ast::generic::Children{.value = newChildren});
}

ELFIO::elfio pas::obj::pep10::createElf() {
  static const char p10mac[2] = {'p', 'x'};
  ELFIO::elfio ret;
  ret.create(ELFIO::ELFCLASS32, ELFIO::ELFDATA2MSB);
  ret.set_os_abi(ELFIO::ELFOSABI_NONE);
  ret.set_type(ELFIO::ET_EXEC);
  ret.set_machine(*p10mac);
  return ret;
}

void writeSymtab(ELFIO::elfio &elf, symbol::Table &table, QString prefix) {
  ELFIO::section *strTab = nullptr;
  for (auto &sec : elf.sections) {
    if (sec->get_name() == ".strtab" && sec->get_type() == ELFIO::SHT_SYMTAB) {
      strTab = sec.get();
      break;
    }
  }
  if (strTab == nullptr) {
    strTab = elf.sections.add(".strtab");
    strTab->set_type(ELFIO::SHT_STRTAB);
  }
  auto symTab = elf.sections.add(u"%1.symtab"_qs.arg(prefix).toStdString());
  symTab->set_type(ELFIO::SHT_SYMTAB);
  symTab->set_info(0);
  symTab->set_addr_align(2);
  symTab->set_entry_size(elf.get_default_entry_size(ELFIO::SHT_SYMTAB));
  symTab->set_link(strTab->get_index());

  ELFIO::string_section_accessor strAc(strTab);
  auto findOrCreateStr = [&](const std::string &str) {
    auto tabStart = strTab->get_data();
    auto tabEnd = tabStart + strTab->get_size();
    auto iter = std::search(tabStart, tabEnd, str.cbegin(), str.cend());
    if (iter != tabEnd)
      return (ELFIO::Elf_Word)(iter - tabStart);
    return strAc.add_string(str.data());
  };
  ELFIO::symbol_section_accessor symAc(elf, symTab);
  for (auto [name, entry] : table.entries()) {
    auto nameIdx = findOrCreateStr(name.toStdString());
    auto secIdx = entry->section_index;
    auto value = entry->value;

    quint8 type = ELFIO::STT_NOTYPE;
    if (value->type() == symbol::Type::kCode)
      type = ELFIO::STT_FUNC;
    else if (value->type() == symbol::Type::kObject)
      type = ELFIO::STT_OBJECT;
    else if (value->type() == symbol::Type::kConstant) {
      type = ELFIO::STT_OBJECT;
      secIdx = ELFIO::SHN_ABS;
    }

    quint8 bind;
    if (entry->binding == symbol::Binding::kLocal)
      bind = ELFIO::STB_LOCAL;
    else if (entry->binding == symbol::Binding::kGlobal)
      bind = ELFIO::STB_GLOBAL;
    else if (entry->binding == symbol::Binding::kImported)
      bind = ELFIO::STB_WEAK;

    quint8 info = (bind << 4) + (type & 0xf);

    if (entry->state == symbol::DefinitionState::kUndefined)
      secIdx = ELFIO::SHN_UNDEF;

    symAc.add_symbol(nameIdx, value->value()(), value->value().byteCount, info,
                     0, secIdx);
  }

  symAc.arrange_local_symbols();
}

void pas::obj::pep10::writeOS(ELFIO::elfio &elf, ast::Node &os) {

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

  for (auto &astSec : ast::children(os)) {
    // Don't emit 0-sized sections.
    auto secName = astSec->get<ast::generic::SectionName>().value;
    auto secFlags = astSec->get<ast::generic::SectionFlags>().value;
    auto traits = pas::ops::generic::detail::getTraits(*astSec);
    auto align = traits.alignment;
    ELFIO::Elf64_Addr baseAddr = traits.base;
    auto size = traits.size;
    auto bytes = pas::ops::pepp::toBytes<isa::Pep10ISA>(*astSec);

    if (size == 0)
      continue; // 0-sized sections are meaningless, do not emit.

    auto sec = elf.sections.add(u"os.%1"_qs.arg(secName).toStdString());
    sec->set_type(ELFIO::SHT_PROGBITS);
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
    if (secFlags.Z)
      getOrCreateBSS(secFlags);
    else
      getOrCreateBits(secFlags);
    activeSeg->add_section(sec, align);
    activeSeg->set_physical_address(
        std::min(activeSeg->get_physical_address(), baseAddr));
    activeSeg->set_virtual_address(
        std::min(activeSeg->get_virtual_address(), baseAddr));
    activeSeg->set_memory_size(activeSeg->get_memory_size() + size);
    for (auto &line : ast::children(*astSec))
      if (line->has<ast::generic::SymbolDeclaration>())
        line->get<ast::generic::SymbolDeclaration>().value->section_index =
            sec->get_index();
  }
  if (os.has<ast::generic::SymbolTable>())
    writeSymtab(elf, *os.get<ast::generic::SymbolTable>().value, u"os"_qs);
  qDebug() << elf.segments.size();
}
