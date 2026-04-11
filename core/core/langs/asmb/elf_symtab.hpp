#pragma once
#include <map>
#include <zpp_bits.h>
#include "codegen.hpp"
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

// Create a lookup data structure that converts IR pointers back to their listing line number.
using IR2ListingLinePair = std::pair<const LinearIR *, u32>;
struct IR2ListingLineComparator {
  bool operator()(const IR2ListingLinePair &lhs, const IR2ListingLinePair &rhs) const { return lhs.first < rhs.first; }
  bool operator()(LinearIR *const lhs, LinearIR *const rhs) const { return lhs < rhs; }
  bool operator()(const LinearIR *const lhs, const LinearIR *const rhs) const { return lhs < rhs; }
};
using IR2ListingLineMap = fc::flat_map<std::vector<IR2ListingLinePair>, IR2ListingLineComparator>;


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
