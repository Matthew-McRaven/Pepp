#pragma once
#include <core/compile/symbol/entry.hpp>
#include <map>
#include <zpp_bits.h>
#include "core/compile/ir_linear/attr_section.hpp"
#include "core/integers.h"
#include "core/langs/asmb/elf_symtab.hpp"
#include "core/langs/asmb/ir_program.hpp"
#include "core/math/bitmanip/leb128.hpp"
#include "fmt/format.h"
#include "ir_attributes.hpp"

namespace ELFIO {
class elfio;
}

namespace pepp::core::symbol {
class LeafTable;
}
namespace pepp::tc {

class DiagnosticTable;

static const SectionDescriptor default_descriptor = {.name = ".text", .flags = SectionFlags(true, true, true, false)};

struct SectionAnalysisResults {
  std::vector<std::pair<SectionDescriptor, IRProgram>> grouped_ir;
};

// The returned vector points to the same underlying IR as the (linear) input program.
// This allows addresses to be propogated to input original. which is useful for generating the listing.
//
// Also extracts system-calls and memory-mapped IO declarations since this is the one time we iterate overthe whole IR
// at once.
SectionAnalysisResults split_to_sections(DiagnosticTable &diag, IRProgram &prog,
                                         SectionDescriptor initial_section = default_descriptor);

// assign_addresses iterates over sections from prog, grouping non-ORG sections contiguously with the nearest ORG
// section to its left. Sections before the first .ORG are an exception, and are grouped with the nearest .ORG to the
// right. When no .ORG is present, all sections are treated as a single group starting at initial_base_address.
//
// For section after a .ORG, the new <base_address> for each section in a section group is the <group_base_address>
// plus the cumulative size of the processed sections in that group after the .ORG, respecting aligment requirements.
// For sections before a .ORG, the new <base_address> is the .ORG minus the cumulative size, iterating right-to-left
// starting in the .ORG section.
//
// When a .BURN <num> is present, grouping occurs as-if an extra section was append to prog which contains a .ORG <num>.
IRMemoryAddressTable<RISCVAddress> assign_addresses(std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                                                    u32 initial_base_address = 0);

ProgramObjectCodeResult to_object_code(const IRMemoryAddressTable<RISCVAddress> &,
                                       std::vector<std::pair<SectionDescriptor, IRProgram>> &prog);

ElfResult to_elf(std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                 const IRMemoryAddressTable<RISCVAddress> &addrs, const ProgramObjectCodeResult &object_code);

} // namespace pepp::tc
