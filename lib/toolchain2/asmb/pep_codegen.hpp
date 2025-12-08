#pragma once
#include "flat/flat_map.hpp"
#include "toolchain/link/mmio.hpp"
#include "toolchain2/asmb/pep_common.hpp"

namespace ELFIO {
class elfio;
}

namespace pepp::tc {

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
SectionAnalysisResults split_to_sections(PepIRProgram &prog, SectionDescriptor initial_section = default_descriptor);

// Assigning addresses should be O(1) and incur O(1) memory allocations.
// A pre-sized vector (essentially an arena allocator) is a natural container.
// A flat_map is a container adaptor which presents a map api over a different ordered container, but you still get a
// convenient O(lgn) find without lots of evil stl magic.
using IRMemoryAddressPair = std::pair<const ir::LinearIR *, ir::attr::Address>;
namespace detail {
// Only compare based on the address of ir::LinearIR *, ignoring ir::attr::Address.
// We already have a guarentee that all IR are unique.
struct IRComparator {
  bool operator()(const IRMemoryAddressPair &lhs, const IRMemoryAddressPair &rhs) const {
    return lhs.first < rhs.first;
  }
  bool operator()(ir::LinearIR *const lhs, ir::LinearIR *const &rhs) const { return lhs < rhs; }
  bool operator()(const ir::LinearIR *const lhs, const ir::LinearIR *const &rhs) const { return lhs < rhs; }
};
} // namespace detail
using IRMemoryAddressTable = fc::flat_map<std::vector<IRMemoryAddressPair>, detail::IRComparator>;

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
struct ProgramObjectCodeResult {
  IR2ObjectCodeMap ir_to_object_code;
  // A common arena for all section's object code and relocations
  std::vector<quint8> object_code;
  std::vector<void *> relocations;
  struct SectionSpans {
    bits::span<quint8> object_code;
    bits::span<void *> relocations;
  };
  // Use section indicies from original "prog"
  // Provides only the object code / relocations for a particular section descriptor.
  std::vector<SectionSpans> section_spans;
};

ProgramObjectCodeResult to_object_code(const IRMemoryAddressTable &,
                                       std::vector<std::pair<SectionDescriptor, PepIRProgram>> &prog);

QSharedPointer<ELFIO::elfio> to_elf(std::vector<std::pair<SectionDescriptor, PepIRProgram>> &prog,
                                    const IRMemoryAddressTable &addrs, const ProgramObjectCodeResult &object_code,
                                    const std::vector<obj::IO> &mmios);

void write_symbol_table(ELFIO::elfio &, symbol::Table &symbol_table, const QString name = ".symtab");
}
