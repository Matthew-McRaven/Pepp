#pragma once
#include <QtCore>
#include <elfio/elfio.hpp>
#include "asm/pas/ast/generic/attr_sec.hpp"
#include "asm/pas/ast/generic/attr_symbol.hpp"
#include "asm/pas/ast/node.hpp"
#include "asm/pas/operations/generic/combine.hpp"
#include "asm/pas/operations/pepp/bytes.hpp"
#include "asm/symbol/entry.hpp"

namespace pas::obj::common {
ELFIO::section *addStrTab(ELFIO::elfio &elf);
struct BinaryLineMapping {
  uint32_t address;
  // 0 indicates unset; so we start counting from 1
  uint16_t srcLine, listLine;
  std::strong_ordering operator<=>(const BinaryLineMapping &other) const { return address <=> other.address; }
  operator QString() const {
    using namespace Qt::StringLiterals;
    static const auto format = u"%1: (%2,%3)"_s;
    static const auto opt = [](uint16_t l) {
      if (l == 0) return u"    "_s;
      else return u"%1"_s.arg((int)l, 4);
    };
    return format.arg(address, 4, 16).arg(opt(srcLine)).arg(opt(listLine));
  }
};
void writeLineMapping(ELFIO::elfio &elf, pas::ast::Node &root);
std::vector<BinaryLineMapping> getLineMappings(ELFIO::elfio &elf);
void writeSymtab(ELFIO::elfio &elf, symbol::Table &table, QString prefix);

template <typename ISA> void writeTree(ELFIO::elfio &elf, pas::ast::Node &node, QString prefix, bool isOS) {
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

  for (auto &astSec : ast::children(node)) {
    // Strip leading . from sections, since we will be adding prefix+".".
    auto secName = astSec->get<ast::generic::SectionName>().value;
    if (secName.startsWith(".")) secName = secName.mid(1);
    auto secFlags = astSec->get<ast::generic::SectionFlags>().value;
    auto traits = pas::ops::generic::detail::getTraits(*astSec);
    auto align = traits.alignment;
    ELFIO::Elf64_Addr baseAddr = traits.base;
    auto size = traits.size;
    auto bytes = pas::ops::pepp::toBytes<ISA>(*astSec);
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
    if (!isOS) activeSeg = elf.segments[0];
    else if (secFlags.Z) getOrCreateBSS(secFlags);
    else getOrCreateBits(secFlags);

    activeSeg->add_section(sec, align);
    activeSeg->set_physical_address(std::min(activeSeg->get_physical_address(), baseAddr));
    activeSeg->set_virtual_address(std::min(activeSeg->get_virtual_address(), baseAddr));

    // Field not re-computed on its own. Failure to compute will cause readelf
    // to crash.
    // TODO: in the future, handle alignment correctly?
    if (isOS) activeSeg->set_memory_size(activeSeg->get_memory_size() + size);

    // Update the section index of all symbols in this section, otherwise symtab
    // will link against SHN_UNDEF.
    for (auto &line : ast::children(*astSec))
      if (line->has<ast::generic::SymbolDeclaration>())
        line->get<ast::generic::SymbolDeclaration>().value->section_index = sec->get_index();
  }

  if (node.has<ast::generic::SymbolTable>()) writeSymtab(elf, *node.get<ast::generic::SymbolTable>().value, prefix);
}

namespace detail {
// Get line mapping section or return nullptr;
ELFIO::section *getLineMappingSection(ELFIO::elfio &elf);
} // namespace detail
} // namespace pas::obj::common
