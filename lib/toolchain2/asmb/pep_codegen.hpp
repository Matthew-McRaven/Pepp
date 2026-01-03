#pragma once
#include "toolchain/link/mmio.hpp"
#include "toolchain/pas/obj/common.hpp"
#include "toolchain2/asmb/pep_common.hpp"

namespace ELFIO {
class elfio;
}

namespace pepp::tc {

class DiagnosticTable;

struct SectionDescriptor {
  std::string name;
  pepp::tc::ir::attr::SectionFlags flags;
  quint16 alignment = 1, org_count = 0;
  std::optional<quint16> base_address = std::nullopt;
  quint16 low_address = 0, high_address = 0;
  // Number of bytes that will be written out to ELF file.
  // Not high_address-low_address because .ORG can mess with address!
  quint16 byte_count = 0;
  // Symbols need to know which section they are going to be relocated against.
  // Rather than wait until the elf file has been generated, we can verify (through source code inspection!) the number
  // that will be assigned to the first non-ELF-plumbing section.
  // Then, splitting to sections can increment this counter AND update the symbol declaration's links.
  static constexpr quint16 section_base_index = 3;
  quint16 section_index = section_base_index;
};

static const SectionDescriptor default_descriptor = {.name = ".text",
                                                     .flags = ir::attr::SectionFlags(true, true, true, false)};

struct SectionAnalysisResults {
  std::vector<std::pair<SectionDescriptor, PepIRProgram>> grouped_ir;
  std::vector<std::string> system_calls;
  std::vector<obj::IO> mmios;
};

// The returned vector points to the same underlying IR as the (linear) input program.
// This allows addresses to be propogated to input original. which is useful for generating the listing.
//
// Also extracts system-calls and memory-mapped IO declarations since this is the one time we iterate overthe whole IR
// at once.
SectionAnalysisResults split_to_sections(DiagnosticTable &diag, PepIRProgram &prog,
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
IRMemoryAddressTable assign_addresses(std::vector<std::pair<SectionDescriptor, PepIRProgram>> &prog,
                                      quint16 initial_base_address = 0);

// Create a lookup data structure that converts IR pointers back to the generated object code.
// Since IR no longer know their own address, we need to cache the object code because it cannot easily be regenerated.
using IR2ObjectPair = std::pair<const ir::LinearIR *, bits::span<quint8>>;
struct IR2ObjectComparator {
  bool operator()(const IR2ObjectPair &lhs, const IR2ObjectPair &rhs) const { return lhs.first < rhs.first; }
  bool operator()(ir::LinearIR *const lhs, ir::LinearIR *const rhs) const { return lhs < rhs; }
  bool operator()(const ir::LinearIR *const lhs, const ir::LinearIR *const rhs) const { return lhs < rhs; }
};
using IR2ObjectCodeMap = fc::flat_map<std::vector<IR2ObjectPair>, IR2ObjectComparator>;
struct PepStaticRelocation {
  // Offset into a section's object code (in bytes) which needs relocation.
  // Per ELF spec, needs to be offset for relocatable object files rather than an address to simplify linker.
  quint32 section_offset;
  quint32 section_idx; // section index in prog, not ELF.
};

struct ProgramObjectCodeResult {
  IR2ObjectCodeMap ir_to_object_code;
  // Group relocations by symbol rather than by section so that we can write the symbol table and relocations
  // simultaneously.
  std::multimap<QSharedPointer<symbol::Entry>, PepStaticRelocation> relocations;
  // A common arena for all section's object code
  std::vector<quint8> object_code;
  struct SectionSpans {
    bits::span<quint8> object_code;
  };
  // Use section indicies from original "prog" and provides only the object code for a particular section descriptor.
  std::vector<SectionSpans> section_spans;
};

ProgramObjectCodeResult to_object_code(const IRMemoryAddressTable &,
                                       std::vector<std::pair<SectionDescriptor, PepIRProgram>> &prog);

// Create a lookup data structure that converts IR pointers back to their listing line number.
using IR2ListingLinePair = std::pair<const ir::LinearIR *, quint16>;
struct IR2ListingLineComparator {
  bool operator()(const IR2ListingLinePair &lhs, const IR2ListingLinePair &rhs) const { return lhs.first < rhs.first; }
  bool operator()(ir::LinearIR *const lhs, ir::LinearIR *const rhs) const { return lhs < rhs; }
  bool operator()(const ir::LinearIR *const lhs, const ir::LinearIR *const rhs) const { return lhs < rhs; }
};
using IR2ListingLineMap = fc::flat_map<std::vector<IR2ListingLinePair>, IR2ListingLineComparator>;

struct ElfResult {
  QSharedPointer<ELFIO::elfio> elf;
  // Some sections in the program are not omitted to to ELF file.
  // However, we pre-computed section indices in the ELF file in split_to_sections and used these in the symbol table.
  // [originally computed section index] is the number you need to subtract to get actual index in the output file.
  std::vector<quint16> section_offsets;
  IR2ListingLineMap ir_to_listing;
};

ElfResult to_elf(std::vector<std::pair<SectionDescriptor, PepIRProgram>> &prog, const IRMemoryAddressTable &addrs,
                 const ProgramObjectCodeResult &object_code, const std::vector<obj::IO> &mmios);

// Write out the symbol table and relocations at the same time.
// Otherwise, we would need to convert all of the symbol::Entry* pointers a second time.
void write_symbol_table(ElfResult &elf, symbol::Table &symbol_table, const ProgramObjectCodeResult &oc,
                        const QString name = ".symtab");
}
