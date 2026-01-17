#include "packed_access_headers.hpp"
#include "packed_elf.hpp"

struct SectionDataVisitor {
  u16 section_index;
  template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E>
  std::shared_ptr<const pepp::bts::AStorage> operator()(const pepp::bts::PackedElf<B, E> *elf) {
    if (section_index >= elf->section_data.size()) throw std::out_of_range("section_data: section index out of range");
    return elf->section_data[section_index];
  }
};
std::shared_ptr<const pepp::bts::AStorage> pepp::bts::section_data(ConstAnyPackedElfPtr elf, u16 section_index) {
  return std::visit(SectionDataVisitor{section_index}, elf);
}

struct ShAlignVisitor {
  u16 section_index;
  template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E> u32 operator()(const pepp::bts::PackedElf<B, E> *elf) {
    if (section_index >= elf->section_headers.size()) throw std::out_of_range("sh_align: section index out of range");
    return static_cast<u32>(elf->section_headers[section_index].sh_addralign);
  }
};
u32 pepp::bts::sh_align(ConstAnyPackedElfPtr elf, u16 section_index) {
  ShAlignVisitor visitor{section_index};
  return std::visit(visitor, elf);
}
