#pragma once
#include <map>
#include <zpp_bits.h>
#include "core/compile/ir_linear/attr_section.hpp"
#include "core/compile/ir_linear/line_base.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/integers.h"
#include "core/math/bitmanip/leb128.hpp"
#include "flat/flat_map.hpp"
#include "fmt/format.h"

namespace ELFIO {
class elfio;
}
namespace pepp::tc {

// Create a lookup data structure that converts IR pointers back to the generated object code.
// Since IR no longer know their own address, we need to cache the object code because it cannot easily be regenerated.
using IR2ObjectPair = std::pair<const LinearIR *, std::span<u8>>;
struct IR2ObjectComparator {
  bool operator()(const IR2ObjectPair &lhs, const IR2ObjectPair &rhs) const { return lhs.first < rhs.first; }
  bool operator()(LinearIR *const lhs, LinearIR *const rhs) const { return lhs < rhs; }
  bool operator()(const LinearIR *const lhs, const LinearIR *const rhs) const { return lhs < rhs; }
};
using IR2ObjectCodeMap = fc::flat_map<std::vector<IR2ObjectPair>, IR2ObjectComparator>;
struct StaticRelocation {
  // Offset into a section's object code (in bytes) which needs relocation.
  // Per ELF spec, needs to be offset for relocatable object files rather than an address to simplify linker.
  u32 section_offset;
  u32 section_idx; // section index in prog, not ELF.
};

// Create a lookup data structure that converts IR pointers back to their listing line number.
using IR2ListingLinePair = std::pair<const LinearIR *, u32>;
struct IR2ListingLineComparator {
  bool operator()(const IR2ListingLinePair &lhs, const IR2ListingLinePair &rhs) const { return lhs.first < rhs.first; }
  bool operator()(LinearIR *const lhs, LinearIR *const rhs) const { return lhs < rhs; }
  bool operator()(const LinearIR *const lhs, const LinearIR *const rhs) const { return lhs < rhs; }
};
using IR2ListingLineMap = fc::flat_map<std::vector<IR2ListingLinePair>, IR2ListingLineComparator>;

struct ProgramObjectCodeResult {
  IR2ObjectCodeMap ir_to_object_code;
  // Group relocations by symbol rather than by section so that we can write the symbol table and relocations
  // simultaneously.
  std::multimap<std::shared_ptr<pepp::core::symbol::Entry>, StaticRelocation> relocations;
  // A common arena for all section's object code
  std::vector<u8> object_code;
  struct SectionSpans {
    std::span<u8> object_code;
  };
  // Use section indicies from original "prog" and provides only the object code for a particular section descriptor.
  std::vector<SectionSpans> section_spans;
};

struct SectionDescriptor {
  std::string name;
  pepp::tc::SectionFlags flags;
  u16 alignment = 1, org_count = 0;
  std::optional<u16> base_address = std::nullopt;
  u32 low_address = 0, high_address = 0;
  // Number of bytes that will be written out to ELF file.
  // Not high_address-low_address because .ORG can mess with address!
  u32 byte_count = 0;
  // Symbols need to know which section they are going to be relocated against.
  // Rather than wait until the elf file has been generated, we can verify (through source code inspection!) the number
  // that will be assigned to the first non-ELF-plumbing section.
  // Then, splitting to sections can increment this counter AND update the symbol declaration's links.
  static constexpr u16 section_base_index = 3;
  u16 section_index = section_base_index;
};

struct ElfResult {
  std::shared_ptr<ELFIO::elfio> elf;
  // Some sections in the program are not omitted to to ELF file.
  // However, we pre-computed section indices in the ELF file in split_to_sections and used these in the symbol table.
  // [originally computed section index] is the number you need to subtract to get actual index in the output file.
  std::vector<u16> section_offsets;
  IR2ListingLineMap ir_to_listing;
};

// Write out the symbol table and relocations at the same time.
// Otherwise, we would need to convert all of the symbol::Entry* pointers a second time.
void write_symbol_table(ElfResult &elf, pepp::core::symbol::LeafTable &symbol_table, const ProgramObjectCodeResult &oc,
                        const std::string name = ".symtab");
} // namespace pepp::tc
