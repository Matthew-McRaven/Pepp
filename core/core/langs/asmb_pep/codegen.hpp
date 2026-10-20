#pragma once
#include "core/compile/ir_linear/line_macro.hpp"
#include "core/integers.h"
#include "core/langs/asmb/elf_symtab.hpp"
#include "core/langs/asmb/ir_constraints.hpp"
#include "core/langs/asmb/ir_program.hpp"
#include "core/langs/asmb/mmio.hpp"
#include "ir_attributes.hpp"

namespace pepp::core::symbol {
class LeafTable;
}
namespace pepp::tc {

class DiagnosticTable;

static const SectionDescriptor default_descriptor = {.name = ".text", .flags = SectionFlags(true, true, true, false)};
// Bytes addressable by a 16-bit Pep address.
static constexpr u64 pep_address_space_size = 0x10000;

struct PeppSectionAnalysisResults {
  std::vector<std::pair<SectionDescriptor, IRProgram>> grouped_ir;
  std::vector<std::string> system_calls;
  std::vector<IO> mmios;
};

// Expects that tree-like IR has already been flattened in a previous step.
// See pepp::tc::parser::flatten_macros.
// The returned vector points to the same underlying IR as the (linear) input program.
// This allows addresses to be propogated to input original. which is useful for generating the listing.
//
// Also extracts system-calls and memory-mapped IO declarations since this is the one time we iterate overthe whole IR
// at once.
PeppSectionAnalysisResults pepp_split_to_sections(DiagnosticTable &diag, IRProgram &prog,
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
IRMemoryAddressTable<PeppAddress> pepp_assign_addresses(DiagnosticTable &diag,
                                                        std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                                                        u16 initial_base_address = 0);

// The constraint for a line's argument.
std::optional<ArgumentConstraint> pepp_argument_constraint(const LinearIR &line);

// Object code larger than the address space is an error, and produces no object code. Undefined symbols are left as
// relocations.
ProgramObjectCodeResult pepp_to_object_code(DiagnosticTable &diag, const IRMemoryAddressTable<PeppAddress> &,
                                            std::vector<std::pair<SectionDescriptor, IRProgram>> &prog);

// `source_order` holds the lines of `prog` in source order, which is the order the listing prints them.
ElfResult pepp_to_elf(std::vector<std::pair<SectionDescriptor, IRProgram>> &prog, const IRProgram &source_order,
                      const IRMemoryAddressTable<PeppAddress> &addrs, const ProgramObjectCodeResult &object_code,
                      const pepp::core::symbol::LeafTable &symbols, const std::vector<IO> &mmios,
                      const DebugLineFiles &files = {},
                      const std::vector<std::shared_ptr<const InlineMacroDefinition>> &macros = {});
} // namespace pepp::tc
