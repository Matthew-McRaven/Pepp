#pragma once
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
void assign_addresses(PepIRProgram &prog);
}
