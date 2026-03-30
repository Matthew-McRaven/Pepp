#pragma once
#include "core/langs/asmb_riscv/ir_attributes.hpp"
#include "core/langs/asmb_riscv/ir_lines.hpp"
#include "flat/flat_map.hpp"

namespace pepp::tc {
using RISCVIRProgram = std::vector<std::shared_ptr<tc::LinearIR>>;

// Assigning addresses should be O(1) and incur O(1) memory allocations.
// A pre-sized vector (essentially an arena allocator) is a natural container.
// A flat_map is a container adaptor which presents a map api over a different ordered container, but you still get a
// convenient O(lgn) find without lots of evil stl magic.
using IRMemoryAddressPair = std::pair<const LinearIR *, Address>;
namespace detail {
// Only compare based on the address of ir::LinearIR *, ignoring ir::attr::Address.
// We already have a guarentee that all IR are unique.
struct IRComparator {
  bool operator()(const IRMemoryAddressPair &lhs, const IRMemoryAddressPair &rhs) const {
    return lhs.first < rhs.first;
  }
  bool operator()(LinearIR *const lhs, LinearIR *const &rhs) const { return lhs < rhs; }
  bool operator()(const LinearIR *const lhs, const LinearIR *const &rhs) const { return lhs < rhs; }
};
} // namespace detail
using IRMemoryAddressTable = fc::flat_map<std::vector<IRMemoryAddressPair>, detail::IRComparator>;
} // namespace pepp::tc
