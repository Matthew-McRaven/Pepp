#include "core/langs/asmb_riscv/codegen.hpp"
#include <elfio/elfio.hpp>
#include <fmt/format.h>
#include "core/compile/ir_linear/attr_symbol.hpp"
#include "core/langs/asmb/codegen.hpp"
#include "core/langs/asmb_riscv/ir_lines.hpp"

pepp::tc::RISCVSectionAnalysisResults pepp::tc::riscv_split_to_sections(DiagnosticTable &diag, IRProgram &prog,
                                                                        SectionDescriptor initial_section) {
  RISCVSectionAnalysisResults ret;
  ret.grouped_ir.emplace_back(std::make_pair(initial_section, pepp::tc::IRProgram{}));
  auto &grouped_ir = ret.grouped_ir;
  auto *active = &grouped_ir[0];
  for (auto &line : prog) {
    if (auto symbol_attr = line->typed_attribute<SymbolDeclaration>(); symbol_attr) {
      if (!symbol_attr->entry->is_singly_defined())
        throw std::logic_error(fmt::format("Multiply defined symbol {}", symbol_attr->entry->name));
      // Symbols need to know their defining section to enable relocations.
      symbol_attr->entry->section_index = active->first.section_index;
    }
    active->second.emplace_back(line);
  }
  return ret;
}
pepp::tc::IRMemoryAddressTable<pepp::tc::RISCVAddress>
pepp::tc::riscv_assign_addresses(std::vector<std::pair<SectionDescriptor, IRProgram>> &prog, u32 initial_base_address) {
  static const auto f = [](const pepp::tc::LinearIR *line) -> pepp::core::symbol::Type {
    auto isCode = dynamic_cast<const IntegerInstruction *>(&*line);
    return isCode ? pepp::core::symbol::Type::Code : pepp::core::symbol::Type::Object;
  };
  return assign_addresses<RISCVAddress>(prog, f, initial_base_address);
}
