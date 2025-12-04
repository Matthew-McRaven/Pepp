#pragma once
#include "flat/flat_map.hpp"
#include "toolchain2/asmb/pep_common.hpp"

namespace pepp::tc {

struct SectionDescriptor {
  std::string name;
  pepp::tc::ir::attr::SectionFlags flags;
  quint16 alignment = 1;
  std::optional<quint16> base_address = std::nullopt;
};

static const SectionDescriptor default_descriptor = {.name = ".text",
                                                     .flags = ir::attr::SectionFlags(true, true, true, false)};

// The returned vector points to the same underlying IR as the (linear) input program.
// This allows addresses to be propogated to input original. which is useful for generating the listing.
std::vector<std::pair<SectionDescriptor, PepIRProgram>>
split_to_sections(PepIRProgram &prog, SectionDescriptor initial_section = default_descriptor);

using IRMemoryAddressPair = std::pair<ir::LinearIR *, ir::attr::Address>;
namespace detail {
struct IRComparator {
  bool operator()(const IRMemoryAddressPair &lhs, const IRMemoryAddressPair &rhs) const {
    return lhs.first < rhs.first;
  }
  bool operator()(ir::LinearIR *const lhs, ir::LinearIR *const &rhs) const { return lhs < rhs; }
};
} // namespace detail
using IRMemoryAddressTable = fc::flat_map<std::vector<IRMemoryAddressPair>, detail::IRComparator>;
// Assigns addresses to each section, usually starting from 0.
// If base_address is not empty, addresses are assigned back-to-front.
// Otherwise, they are assigned front-to-back.
// An .ORG will immediately set the base_address to the argument's value.
// A separate step (i.e., linking) is required to combine the sections into a single address space.
IRMemoryAddressTable assign_addresses(std::vector<std::pair<SectionDescriptor, PepIRProgram>> &prog);
}
