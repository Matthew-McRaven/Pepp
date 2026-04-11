#include "core/langs/asmb_riscv/codegen.hpp"
#include <elfio/elfio.hpp>
#include <fmt/format.h>
#include "core/compile/ir_linear/attr_symbol.hpp"
#include "core/langs/asmb/codegen.hpp"
#include "core/langs/asmb_riscv/ir_lines.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "ir_visitor.hpp"

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

namespace pepp::tc {
struct RISCVObjectVistitor : public RISCVIRVisitor {
  const IRMemoryAddressTable<RISCVAddress> &ir_to_address;
  const u32 base_address, section_idx;
  // On each call, out_bytes will be shortened by the size of the visited line;
  bits::span<u8> out_bytes;
  std::multimap<std::shared_ptr<pepp::core::symbol::Entry>, StaticRelocation> &relocations;
  IR2ObjectCodeMap &ir_to_object_code;
  RISCVObjectVistitor(const IRMemoryAddressTable<RISCVAddress> &, const u32 base_address, const u16 section_idx,
                      bits::span<u8>, std::multimap<std::shared_ptr<pepp::core::symbol::Entry>, StaticRelocation> &,
                      IR2ObjectCodeMap &);
  void visit(const EmptyLine *) override;
  void visit(const CommentLine *) override;
  void visit(const RTypeIR *) override;
  void visit(const ITypeIR *) override;
  void visit(const STypeIR *) override;
  void visit(const BTypeIR *) override;
  void visit(const UTypeIR *) override;
  void visit(const JTypeIR *) override;
  void visit(const DotAlign *) override;
  void visit(const DotLiteral *) override;
  void visit(const DotBlock *) override;
  void visit(const DotEquate *) override;
  void visit(const DotSection *) override;
  void visit(const DotOrg *) override;
};

pepp::tc::RISCVObjectVistitor::RISCVObjectVistitor(
    const IRMemoryAddressTable<RISCVAddress> &ir_to_address, const u32 base_address, const u16 section_idx,
    bits::span<u8> out_bytes, std::multimap<std::shared_ptr<pepp::core::symbol::Entry>, StaticRelocation> &relocs,
    IR2ObjectCodeMap &ir_to_object_code)
    : ir_to_address(ir_to_address), base_address(base_address), section_idx(section_idx), out_bytes(out_bytes),
      relocations(relocs), ir_to_object_code(ir_to_object_code) {}

void pepp::tc::RISCVObjectVistitor::visit(const EmptyLine *) {
  // Does not generate object code
}

void pepp::tc::RISCVObjectVistitor::visit(const CommentLine *) {
  // Does not generate object code
}

void pepp::tc::RISCVObjectVistitor::visit(const RTypeIR *line) {
  riscv::Values vals{.rs1 = line->rs1, .rs2 = line->rs2, .rd = line->rd, .imm = std::nullopt};
  auto encoded = line->mnemonic.mn.encode(vals).bits();
  bits::span<const u8> span{(const u8 *)&encoded, 4};
  bits::memcpy_endian(out_bytes.first(4), bits::Order::LittleEndian, span, bits::hostOrder());
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(4)});
  out_bytes = out_bytes.subspan(4);
}

void pepp::tc::RISCVObjectVistitor::visit(const ITypeIR *line) {
  auto imm = line->imm->value_as<u32>();
  riscv::Values vals{.rs1 = line->rs1, .rs2 = std::nullopt, .rd = line->rd, .imm = imm};
  auto encoded = line->mnemonic.mn.encode(vals).bits();
  bits::span<const u8> span{(const u8 *)&encoded, 4};
  bits::memcpy_endian(out_bytes.first(4), bits::Order::LittleEndian, span, bits::hostOrder());
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(4)});
  out_bytes = out_bytes.subspan(4);
}

void pepp::tc::RISCVObjectVistitor::visit(const STypeIR *line) {
  auto imm = line->imm->value_as<u32>();
  riscv::Values vals{.rs1 = line->rs1, .rs2 = line->rs2, .rd = std::nullopt, .imm = imm};
  auto encoded = line->mnemonic.mn.encode(vals).bits();
  bits::span<const u8> span{(const u8 *)&encoded, 4};
  bits::memcpy_endian(out_bytes.first(4), bits::Order::LittleEndian, span, bits::hostOrder());
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(4)});
  out_bytes = out_bytes.subspan(4);
}

void pepp::tc::RISCVObjectVistitor::visit(const BTypeIR *line) {
  auto imm = line->imm->value_as<u32>();
  riscv::Values vals{.rs1 = line->rs1, .rs2 = line->rs2, .rd = std::nullopt, .imm = imm};
  auto encoded = line->mnemonic.mn.encode(vals).bits();
  bits::span<const u8> span{(const u8 *)&encoded, 4};
  bits::memcpy_endian(out_bytes.first(4), bits::Order::LittleEndian, span, bits::hostOrder());
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(4)});
  out_bytes = out_bytes.subspan(4);
}

void pepp::tc::RISCVObjectVistitor::visit(const UTypeIR *line) {
  auto imm = line->imm->value_as<u32>();
  riscv::Values vals{.rs1 = std::nullopt, .rs2 = std::nullopt, .rd = line->rd, .imm = imm};
  auto encoded = line->mnemonic.mn.encode(vals).bits();
  bits::span<const u8> span{(const u8 *)&encoded, 4};
  bits::memcpy_endian(out_bytes.first(4), bits::Order::LittleEndian, span, bits::hostOrder());
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(4)});
  out_bytes = out_bytes.subspan(4);
}

void pepp::tc::RISCVObjectVistitor::visit(const JTypeIR *line) {
  auto imm = line->imm->value_as<u32>();
  riscv::Values vals{.rs1 = std::nullopt, .rs2 = std::nullopt, .rd = line->rd, .imm = imm};
  auto encoded = line->mnemonic.mn.encode(vals).bits();
  bits::span<const u8> span{(const u8 *)&encoded, 4};
  bits::memcpy_endian(out_bytes.first(4), bits::Order::LittleEndian, span, bits::hostOrder());
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(4)});
  out_bytes = out_bytes.subspan(4);
}

void pepp::tc::RISCVObjectVistitor::visit(const DotAlign *line) {
  auto addr_info = ir_to_address.at(line);
  std::ranges::fill(out_bytes.first(addr_info.size), 0);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::RISCVObjectVistitor::visit(const DotLiteral *line) {
  auto addr_info = ir_to_address.at(line);
  // Emit relocations for undefined symbolic arguments.
  auto as_symbolic_arg = std::dynamic_pointer_cast<pepp::ast::Symbolic>(line->argument.value);
  if (as_symbolic_arg != nullptr) {
    auto symbol = as_symbolic_arg->symbol();
    if (symbol->is_undefined()) {
      u16 offset = addr_info.address - base_address;
      relocations.insert({symbol, StaticRelocation{.section_offset = offset, .section_idx = section_idx}});
    }
  }
  (void)line->argument.value->serialize(out_bytes.first(addr_info.size), bits::Order::BigEndian);

  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::RISCVObjectVistitor::visit(const DotBlock *line) {
  auto addr_info = ir_to_address.at(line);
  std::ranges::fill(out_bytes.first(addr_info.size), 0);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::RISCVObjectVistitor::visit(const DotEquate *) {
  // Does not generate object code
}

void pepp::tc::RISCVObjectVistitor::visit(const DotSection *) {
  // Does not generate object code
}

void pepp::tc::RISCVObjectVistitor::visit(const DotOrg *) {
  // Does not generate object code
}

} // namespace pepp::tc
pepp::tc::ProgramObjectCodeResult
pepp::tc::riscv_to_object_code(const IRMemoryAddressTable<RISCVAddress> &addresses,
                               std::vector<std::pair<SectionDescriptor, IRProgram>> &prog) {
  return to_object_code<RISCVAddress, RISCVObjectVistitor>(addresses, prog);
}
