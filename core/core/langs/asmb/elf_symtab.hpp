#pragma once
#include <map>
#include <zpp_bits.h>
#include "codegen.hpp"
#include "core/compile/ir_linear/attr_section.hpp"
#include "core/compile/ir_linear/line_base.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/formats/elf/packed_elf.hpp"
#include "core/integers.h"
#include "core/math/bitmanip/leb128.hpp"
#include "flat/flat_map.hpp"
#include "fmt/format.h"

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
  // Sections only for now: no segments, and no symbol table.
  pepp::bts::AnyGrowableElf elf;
  IR2ListingLineMap ir_to_listing;
};

// One section per entry of `prog`, written straight into a packed file. Shared by every architecture, since only the
// file header differs between them.
ElfResult sections_to_elf(pepp::bts::ElfBits bits, pepp::bts::ElfEndian endian, pepp::bts::ElfMachineType machine,
                          const std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                          const ProgramObjectCodeResult &object_code);

// Lay the file out and return its bytes. Empty if there is no file.
std::vector<u8> elf_bytes(ElfResult &result);
} // namespace pepp::tc
