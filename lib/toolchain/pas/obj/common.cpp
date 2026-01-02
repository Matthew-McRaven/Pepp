#include "./common.hpp"
#include <elfio/elfio.hpp>
#include <zpp_bits.h>
#include "toolchain/pas/operations/generic/addr2line.hpp"
#include "toolchain/symbol/table.hpp"

static const auto strTabStr = ".strtab";

ELFIO::section *pas::obj::common::addStrTab(ELFIO::elfio &elf) {
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

void pas::obj::common::writeSymtab(ELFIO::elfio &elf, symbol::Table &table, QString prefix) {
  using namespace Qt::StringLiterals;
  auto strTab = pas::obj::common::addStrTab(elf);

  auto symTab = elf.sections.add(u"%1.symtab"_s.arg(prefix).toStdString());
  symTab->set_type(ELFIO::SHT_SYMTAB);
  symTab->set_info(0);
  symTab->set_addr_align(2);
  symTab->set_entry_size(elf.get_default_entry_size(ELFIO::SHT_SYMTAB));
  symTab->set_link(strTab->get_index());

  // Attempt to pool strings when possible, to reduce final binary size.
  // Probably O(n^2), but n should be small for Pep/N.
  ELFIO::string_section_accessor strAc(strTab);
  auto findOrCreateStr = [&](const std::string &str) {
    auto tabStart = strTab->get_data();
    auto tabEnd = tabStart + strTab->get_size();
    // Must use data/size+1 and not begin/end because we MUST include trailing null.
    // Otherwise, `main` is pooled with `mainCln`, which is wrong.
    auto iter = std::search(tabStart, tabEnd, str.data(), str.data() + str.size() + 1);
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

void pas::obj::common::writeLineMapping(ELFIO::elfio &elf, ast::Node &root) {
  auto source = pas::ops::generic::source2addr(root);
  auto list = pas::ops::generic::list2addr(root);

  std::map<uint32_t, std::tuple<uint16_t, uint16_t>> mapping;
  for (const auto &pair : source) {
    auto [line, address] = pair;
    mapping[address] = {line, 0};
  }
  for (const auto &pair : list) {
    auto [line, address] = pair;
    auto it = mapping.find(address);
    if (it != mapping.end()) std::get<1>(it->second) = line;
    else mapping[address] = {0, line};
  }
  auto lineNumbers = detail::getLineMappingSection(elf);
  if (lineNumbers == nullptr) {
    lineNumbers = elf.sections.add(lineMapStr);
    lineNumbers->set_type(ELFIO::SHT_PROGBITS);
  }
  auto [data, in, out] = zpp::bits::data_in_out();

  BinaryLineMapping prev;
  bool first = true;
  for (const auto &[addr, pair] : mapping) {
    auto [src, list] = pair;
    auto current = BinaryLineMapping{.address = (uint32_t)addr, .srcLine = (uint16_t)src, .listLine = (uint16_t)list};
    (void)current.serialize(out, current, &prev, first);
    prev = current, first = false;
  }
  lineNumbers->append_data((const char *)data.data(), out.position());
}

std::vector<pas::obj::common::BinaryLineMapping> pas::obj::common::getLineMappings(ELFIO::elfio &elf) {
  auto lineNumbers = detail::getLineMappingSection(elf);
  if (lineNumbers == nullptr) return {};
  auto data = std::span<const char>(lineNumbers->get_data(), (std::size_t)lineNumbers->get_size());
  zpp::bits::in in(data);
  std::vector<BinaryLineMapping> ret;
  BinaryLineMapping prev;
  while (in.position() < data.size()) {
    BinaryLineMapping current;
    (void)current.serialize(in, current, &prev);
    ret.push_back(prev = current);
  }
  return ret;
}

ELFIO::section *pas::obj::common::detail::getLineMappingSection(ELFIO::elfio &elf) {
  ELFIO::section *lineNumbers = nullptr;
  for (auto &sec : elf.sections) {
    if (sec->get_name() == lineMapStr && sec->get_type() == ELFIO::SHT_PROGBITS) {
      lineNumbers = sec.get();
      break;
    }
  }
  return lineNumbers;
}
