#include "./pep_ir_visitor.hpp"
#include "./pep_format.hpp"
#include "./pep_ir.hpp"
#include "utils/bits/copy.hpp"
#include "utils/bits/swap.hpp"

void pepp::tc::ir::SourceVisitor::visit(const EmptyLine *) { text = ""; }

void pepp::tc::ir::SourceVisitor::visit(const CommentLine *line) {
  auto comment = ";" + line->comment.to_string();
  text = format_as_columns(comment, "", "", "");
}

void pepp::tc::ir::SourceVisitor::visit(const MonadicInstruction *line) {
  QString symbol = "", mn = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  mn = isa::Pep10::string(line->mnemonic.instruction);
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, mn, "", comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DyadicInstruction *line) {
  QStringList arg_list;
  QString symbol = "", mn = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  mn = isa::Pep10::string(line->mnemonic.instruction);
  arg_list.emplaceBack(line->argument.value->string());
  auto addr_mode = line->addr_mode.addr_mode;
  if (!isa::Pep10::canElideAddressingMode(line->mnemonic.instruction, addr_mode))
    arg_list.emplaceBack(isa::Pep10::string(addr_mode));
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, mn, arg_list.join(","), comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DotAlign *line) {
  QString symbol = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, ".ALIGN", line->argument.value->string(), comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DotLiteral *line) {
  QString symbol = "", dot = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  switch (line->which) {
  case DotLiteral::Which::ASCII: dot = ".ASCII"; break;
  case DotLiteral::Which::Byte: dot = ".BYTE"; break;
  case DotLiteral::Which::Word: dot = ".WORD"; break;
  }

  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, dot, line->argument.value->string(), comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DotBlock *line) {
  QString symbol = "", comment = "";
  if (auto maybe_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); maybe_symbol)
    symbol = maybe_symbol->entry->name + ":";
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(symbol, ".BLOCK", line->argument.value->string(), comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DotEquate *line) {
  QString comment = "";
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns(line->symbol.entry->name + ":", ".EQUATE", line->argument.value->string(), comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DotSection *line) {
  using namespace Qt::StringLiterals;
  QStringList args;
  args.emplaceBack(u"\"%1\""_s.arg(line->name.to_string()));
  args.emplaceBack(u"\"%1\""_s.arg(line->flags.to_string()));
  QString comment = "";
  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns("", ".SECTION", args.join(", "), comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DotAnnotate *line) {
  QString dot = "", comment = "";
  switch (line->which) {
  case DotAnnotate::Which::EXPORT: dot = ".EXPORT"; break;
  case DotAnnotate::Which::IMPORT: dot = ".IMPORT"; break;
  case DotAnnotate::Which::INPUT: dot = ".INPUT"; break;
  case DotAnnotate::Which::OUTPUT: dot = ".OUTPUT"; break;
  case DotAnnotate::Which::SCALL: dot = ".SCALL"; break;
  }

  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns("", dot, line->argument.value->string(), comment);
}

void pepp::tc::ir::SourceVisitor::visit(const DotOrg *line) {
  QString dot = "", comment = "";
  switch (line->behavior) {
  case DotOrg::Behavior::BURN: dot = ".BURN"; break;
  case DotOrg::Behavior::ORG: dot = ".ORG"; break;
  }

  if (auto maybe_comment = line->typed_attribute<ir::attr::Comment>(); maybe_comment)
    comment = ";" + maybe_comment->to_string();
  text = format_as_columns("", dot, line->argument.value->string(), comment);
}

QString pepp::tc::ir::format_source(const LinearIR *line) {
  SourceVisitor r;
  line->accept(&r);
  return r.text;
}

pepp::tc::ir::ObjectCodeVisitor::ObjectCodeVisitor(const IRMemoryAddressTable &ir_to_address,
                                                   bits::span<quint8> out_bytes, std::vector<void *> &relocs,
                                                   IR2ObjectCodeMap &ir_to_object_code)
    : ir_to_address(ir_to_address), out_bytes(out_bytes), relocations(relocs), ir_to_object_code(ir_to_object_code) {}

void pepp::tc::ir::ObjectCodeVisitor::visit(const EmptyLine *) {}

void pepp::tc::ir::ObjectCodeVisitor::visit(const CommentLine *) {}

void pepp::tc::ir::ObjectCodeVisitor::visit(const MonadicInstruction *line) {
  out_bytes[0] = isa::Pep10::opcode(line->mnemonic.instruction);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(1)});
  out_bytes = out_bytes.subspan(1);
}

void pepp::tc::ir::ObjectCodeVisitor::visit(const DyadicInstruction *line) {
  out_bytes[0] = isa::Pep10::opcode(line->mnemonic.instruction, line->addr_mode.addr_mode);
  line->argument.value->value(out_bytes.subspan(1).first(2), bits::Order::BigEndian);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(3)});
  out_bytes = out_bytes.subspan(3);
}

void pepp::tc::ir::ObjectCodeVisitor::visit(const DotAlign *line) {
  auto addr_info = ir_to_address.at(static_cast<const DotAlign *const>(line));
  std::ranges::fill(out_bytes.first(addr_info.size), 0);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::ir::ObjectCodeVisitor::visit(const DotLiteral *line) {
  auto addr_info = ir_to_address.at(static_cast<const DotLiteral *const>(line));
  line->argument.value->value(out_bytes.first(addr_info.size), bits::Order::BigEndian);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::ir::ObjectCodeVisitor::visit(const DotBlock *line) {
  auto addr_info = ir_to_address.at(static_cast<const DotBlock *const>(line));
  std::ranges::fill(out_bytes.first(addr_info.size), 0);
  ir_to_object_code.container.emplace_back(IR2ObjectPair{line, out_bytes.first(addr_info.size)});
  out_bytes = out_bytes.subspan(addr_info.size);
}

void pepp::tc::ir::ObjectCodeVisitor::visit(const DotEquate *) {}

void pepp::tc::ir::ObjectCodeVisitor::visit(const DotSection *) {}

void pepp::tc::ir::ObjectCodeVisitor::visit(const DotAnnotate *) {}

void pepp::tc::ir::ObjectCodeVisitor::visit(const DotOrg *) {}

namespace {
struct SectionOffsets {
  size_t object_code_offset = 0, object_code_size = 0;
  size_t reloc_offset = 0, reloc_size = 0;
};
} // namespace
pepp::tc::ir::ProgramObjectCodeResult
pepp::tc::ir::to_object_code(const IRMemoryAddressTable &addresses,
                             std::vector<std::pair<SectionDescriptor, PepIRProgram>> &prog) {
  ProgramObjectCodeResult ret;
  using Item = std::pair<SectionDescriptor, PepIRProgram>;
  std::vector<SectionOffsets> offsets(prog.size(), SectionOffsets{});
  quint32 object_size = 0, ir_count = 0;
  for (int it = 0; it < prog.size(); it++) {
    const auto &sec = prog[it];
    if (sec.first.flags.z) continue; // No bytes in ELF for Z section; no relocations possible.
    offsets[it].object_code_size = sec.first.byte_count;
    offsets[it].object_code_offset = object_size;
    object_size += sec.first.byte_count;
    ir_count += sec.second.size();
  }
  ret.object_code.resize(object_size, 0);
  ret.ir_to_object_code.container.reserve(ir_count);
  ret.section_spans.reserve(prog.size());

  for (int it = 0; it < prog.size(); it++) {
    const auto &sec = prog[it];
    auto &offset = offsets[it];
    auto code_begin = ret.object_code.begin() + offset.object_code_offset;
    auto code_end = code_begin + offset.object_code_size;

    auto oc_subspan = bits::span<quint8>(code_begin, code_end);
    ObjectCodeVisitor visitor(addresses, oc_subspan, ret.relocations, ret.ir_to_object_code);
    offset.reloc_offset = ret.relocations.size();
    for (const auto &line : sec.second) line->accept(&visitor);
    offset.reloc_size = offset.reloc_offset - ret.relocations.size();
  }

  // SectionInfo cannot be created until core loop is complete, because relocation might re-allocate and invalidate
  // relocation info.
  using SectionSpans = ProgramObjectCodeResult::SectionSpans;
  for (int it = 0; it < prog.size(); it++) {
    auto &offset = offsets[it];
    const auto &sec = prog[it];
    // Z sections need entries in section_spans, but those entries should be empty.
    if (sec.first.flags.z) {
      ret.section_spans.emplace_back(SectionSpans{{}, {}});
    } else {
      auto code_begin = ret.object_code.begin() + offset.object_code_offset;
      auto code_end = code_begin + offset.object_code_size;
      auto oc_subspan = bits::span<quint8>(code_begin, code_end);
      auto reloc_begin = ret.relocations.begin() + offset.reloc_offset;
      auto reloc_end = reloc_begin + offset.reloc_size;
      auto reloc_subspan = bits::span<void *>(reloc_begin, reloc_end);
      ret.section_spans.emplace_back(SectionSpans{oc_subspan, reloc_subspan});
    }
  }

  //  Establish flat-map invariant
  std::sort(ret.ir_to_object_code.container.begin(), ret.ir_to_object_code.container.end(), IR2ObjectComparator{});
  return ret;
}
