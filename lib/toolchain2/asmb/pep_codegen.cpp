#include "./pep_codegen.hpp"
#include <elfio/elfio.hpp>

std::vector<std::pair<pepp::tc::SectionDescriptor, pepp::tc::PepIRProgram>>
pepp::tc::split_to_sections(PepIRProgram &prog, SectionDescriptor initial_section) {
  using Pair = std::pair<pepp::tc::SectionDescriptor, pepp::tc::PepIRProgram>;
  std::vector<Pair> ret;
  ret.emplace_back(std::make_pair(initial_section, pepp::tc::PepIRProgram{}));
  auto *active = &ret[0];
  for (auto &line : prog) {
    auto as_section = std::dynamic_pointer_cast<pepp::tc::ir::DotSection>(line);
    // If no existing section has the same name, create a new section with the provided flags.
    // When the section already exists, ensure that the flags match before switching to that section,
    if (as_section) {
      auto flags = as_section->flags;
      auto name = as_section->name.to_string();
      auto existing_sec = std::find_if(ret.begin(), ret.end(), [&name](auto &i) { return i.first.name == name; });
      if (existing_sec == ret.end()) {
        pepp::tc::SectionDescriptor desc{.name = name.toStdString(), .flags = flags};
        ret.emplace_back(std::make_pair(desc, pepp::tc::PepIRProgram{}));
        active = &ret.back();
      } else if (existing_sec->first.flags != flags) {
        throw std::logic_error("Modifying flags for an existing section");
      } else active = &*existing_sec;
    }
    // TODO: record alignmnt, .BURN, .ORG for this section.
    active->second.emplace_back(line);
  }
  return ret;
}

void pepp::tc::assign_addresses(PepIRProgram &prog) {
  quint16 base_address = 0;
  // if (burn) base_address = *burn;
  for (auto &line : prog) {
    // Register system calls
    // Gather IOs
    // Check that all symbol declarations are singly defined
    // (optional) Check all symbol usages are not undefined
    // Check that all .EQUATE have a symbol.
  }
}
