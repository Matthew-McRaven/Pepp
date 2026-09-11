#include "elf_symtab.hpp"
#include <stdexcept>
#include "core/formats/elf/packed_ops.hpp"
#include "spdlog/spdlog.h"

namespace {
using namespace pepp::bts;

template <ElfBits B, ElfEndian E>
std::unique_ptr<PackedGrowableElfFile<B, E>>
build(ElfMachineType machine, const std::vector<std::pair<pepp::tc::SectionDescriptor, pepp::tc::IRProgram>> &prog,
      const pepp::tc::ProgramObjectCodeResult &object_code) {
  auto elf = std::make_unique<PackedGrowableElfFile<B, E>>(ElfFileType::ET_EXEC, machine, ElfABI::ELFOSABI_NONE);
  // The null section and .shstrtab come first; SectionDescriptor::section_base_index accounts for them.
  ensure_section_header_table(*elf);

  for (u32 it = 0; it < prog.size(); it++) {
    const auto &desc = prog[it].first;
    SPDLOG_INFO("{} creating", desc.name);
    const auto type = desc.flags.z ? SectionTypes::SHT_NOBITS : SectionTypes::SHT_PROGBITS;
    const auto index = add_named_section(*elf, desc.name, type);
    if (index != desc.section_index) throw std::logic_error("Mismatch in pre-computed section index");

    auto &shdr = elf->section_headers[index];
    // Every section from the AST is bits in memory, so all of them are allocated.
    auto flags = bits::to_underlying(SectionFlags::SHF_ALLOC);
    if (desc.flags.x) flags |= bits::to_underlying(SectionFlags::SHF_EXECINSTR);
    if (desc.flags.w) flags |= bits::to_underlying(SectionFlags::SHF_WRITE);
    shdr.sh_flags = flags;
    shdr.sh_addr = desc.low_address;
    shdr.sh_addralign = desc.alignment;
    // Layout sizes sections from their data, which NOBITS has none of.
    if (desc.flags.z) shdr.sh_size = desc.high_address - desc.low_address;
    else {
      const auto bytes = object_code.section_spans[it].object_code;
      elf->section_data[index]->append(bits::span<const u8>{bytes.data(), bytes.size()});
    }
  }
  return elf;
}
} // namespace

pepp::tc::ElfResult pepp::tc::sections_to_elf(ElfBits bits, ElfEndian endian, ElfMachineType machine,
                                              const std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                                              const ProgramObjectCodeResult &object_code) {
  using enum ElfBits;
  using enum ElfEndian;
  ElfResult ret;
  if (bits == b32 && endian == le) ret.elf = build<b32, le>(machine, prog, object_code);
  else if (bits == b32) ret.elf = build<b32, be>(machine, prog, object_code);
  else if (endian == le) ret.elf = build<b64, le>(machine, prog, object_code);
  else ret.elf = build<b64, be>(machine, prog, object_code);
  return ret;
}

std::vector<u8> pepp::tc::elf_bytes(ElfResult &result) {
  auto visitor = [](auto &file) -> std::vector<u8> {
    if (!file) return {};
    auto layout = pepp::bts::calculate_layout(*file);
    std::vector<u8> ret(pepp::bts::size_for_layout(layout), 0);
    pepp::bts::write(ret, layout);
    return ret;
  };
  return std::visit(visitor, result.elf);
}
