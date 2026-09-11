#include "elf_symtab.hpp"
#include <algorithm>
#include <optional>
#include <stdexcept>
#include "core/formats/elf/packed_ops.hpp"
#include "spdlog/spdlog.h"

namespace {
using namespace pepp::bts;

// The segment being grown, if any. A constraint names an index range with ascending addresses, so a section may only
// join when it starts exactly where the segment ends; that also keeps address gaps out of the file.
struct Run {
  u32 flags, end;
  bool has_nobits;
};

template <ElfBits B, ElfEndian E>
pepp::tc::ElfResult build(ElfMachineType machine,
                          const std::vector<std::pair<pepp::tc::SectionDescriptor, pepp::tc::IRProgram>> &prog,
                          const pepp::tc::ProgramObjectCodeResult &object_code) {
  pepp::tc::ElfResult ret;
  using File = PackedGrowableElfFile<B, E>;
  using enum ElfFileType;
  using enum ElfABI;
  // emplace returns the alternative it built, so the variant owns the file and we keep a typed pointer to fill it.
  auto *elf = ret.elf.emplace<std::unique_ptr<File>>(std::make_unique<File>(ET_EXEC, machine, ELFOSABI_NONE)).get();
  // The null section and .shstrtab come first; SectionDescriptor::section_base_index accounts for them.
  ensure_section_header_table(*elf);

  std::optional<Run> run;

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
    // Extent as every reader sees it. high_address - low_address can overstate a section once .ORG is involved.
    u32 extent = 0;
    if (desc.flags.z) {
      extent = desc.high_address - desc.low_address;
      shdr.sh_size = extent;
    } else {
      const auto bytes = object_code.section_spans[it].object_code;
      elf->section_data[index]->append(bits::span<const u8>{bytes.data(), bytes.size()});
      extent = static_cast<u32>(bytes.size());
    }
    const u32 end = desc.low_address + extent;

    u32 load_flags = 0;
    if (desc.flags.r) load_flags |= bits::to_underlying(SegmentFlags::PF_R);
    if (desc.flags.w) load_flags |= bits::to_underlying(SegmentFlags::PF_W);
    if (desc.flags.x) load_flags |= bits::to_underlying(SegmentFlags::PF_X);
    const u16 align = std::max<u16>(desc.alignment, 1);
    const bool occupies_memory = extent > 0;
    // File data after a NOBITS would sit past p_filesz. A trailing NOBITS is fine.
    const bool blocked = run && run->has_nobits && !desc.flags.z && occupies_memory;
    // We can extend the run if: matching flags, contiguous  addresses, and no intervening NOBITS.
    if (run && run->flags == load_flags && !blocked && desc.low_address == bits::align_up(run->end, align)) {
      auto &segment = ret.segments.back();
      segment.to_sec = index;
      segment.alignment = std::max(segment.alignment, align);
      run->end = end;
      run->has_nobits |= desc.flags.z;
    } else if (occupies_memory) {
      SegmentLayoutConstraint segment;
      segment.alignment = align;
      segment.from_sec = segment.to_sec = index;
      segment.update_sec_addrs = false; // Addresses come from assign_addresses.
      elf->add_segment(SegmentType::PT_LOAD, static_cast<SegmentFlags>(load_flags));
      ret.segments.push_back(segment);
      run = Run{load_flags, end, desc.flags.z};
    } else { // A section with no memory usage cannot be part of a segment.
      run.reset();
    }
  }

  return ret;
}
} // namespace

pepp::tc::ElfResult pepp::tc::sections_to_elf(ElfBits bits, ElfEndian endian, ElfMachineType machine,
                                              const std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                                              const ProgramObjectCodeResult &object_code) {
  using enum ElfBits;
  using enum ElfEndian;
  if (bits == b32 && endian == le) return build<b32, le>(machine, prog, object_code);
  else if (bits == b32) return build<b32, be>(machine, prog, object_code);
  else if (endian == le) return build<b64, le>(machine, prog, object_code);
  else return build<b64, be>(machine, prog, object_code);
}

std::vector<u8> pepp::tc::elf_bytes(ElfResult &result) {
  auto visitor = [&result](auto &file) -> std::vector<u8> {
    if (!file) return {};
    auto layout = pepp::bts::calculate_layout(*file, &result.segments);
    std::vector<u8> ret(pepp::bts::size_for_layout(layout), 0);
    pepp::bts::write(ret, layout);
    return ret;
  };
  return std::visit(visitor, result.elf);
}
