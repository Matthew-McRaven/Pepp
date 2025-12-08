#pragma once

#include <QString>
#include "toolchain2/asmb/pep_codegen.hpp"
#include "utils/bits/span.hpp"

namespace pepp::tc::ir {
struct LinearIR;
struct EmptyLine;
struct CommentLine;
struct MonadicInstruction;
struct DyadicInstruction;
struct DotAlign;
struct DotLiteral;
struct DotBlock;
struct DotEquate;
struct DotSection;
struct DotAnnotate;
struct DotOrg;
struct LinearIRVisitor {
  virtual void visit(const EmptyLine *) = 0;
  virtual void visit(const CommentLine *) = 0;
  virtual void visit(const MonadicInstruction *) = 0;
  virtual void visit(const DyadicInstruction *) = 0;
  virtual void visit(const DotAlign *) = 0;
  virtual void visit(const DotLiteral *) = 0;
  virtual void visit(const DotBlock *) = 0;
  virtual void visit(const DotEquate *) = 0;
  virtual void visit(const DotSection *) = 0;
  virtual void visit(const DotAnnotate *) = 0;
  virtual void visit(const DotOrg *) = 0;
};

// If you modify this class, you must also modify pep_format.
// You must then also modify the tests proving equivalence between IR and token formatting.
struct SourceVisitor : public LinearIRVisitor {
  QString text;
  void visit(const EmptyLine *) override;
  void visit(const CommentLine *) override;
  void visit(const MonadicInstruction *) override;
  void visit(const DyadicInstruction *) override;
  void visit(const DotAlign *) override;
  void visit(const DotLiteral *) override;
  void visit(const DotBlock *) override;
  void visit(const DotEquate *) override;
  void visit(const DotSection *) override;
  void visit(const DotAnnotate *) override;
  void visit(const DotOrg *) override;
};
QString format_source(const LinearIR *);

// Create a lookup data structure that converts IR pointers back to the generated object code.
// Since IR no longer know their own address, we need to cache the object code because it cannot easily be regenerated.
using IR2ObjectPair = std::pair<const LinearIR *, bits::span<quint8>>;
struct IR2ObjectComparator {
  bool operator()(const IR2ObjectPair &lhs, const IR2ObjectPair &rhs) const { return lhs.first < rhs.first; }
  bool operator()(ir::LinearIR *const lhs, ir::LinearIR *const rhs) const { return lhs < rhs; }
  bool operator()(const ir::LinearIR *const lhs, const ir::LinearIR *const rhs) const { return lhs < rhs; }
};
using IR2ObjectCodeMap = fc::flat_map<std::vector<IR2ObjectPair>, IR2ObjectComparator>;

struct ObjectCodeVisitor : public LinearIRVisitor {
  const IRMemoryAddressTable &ir_to_address;
  // On each call, out_bytes will be shortened by the size of the visited line;
  bits::span<quint8> out_bytes;
  std::vector<void *> &relocations;
  IR2ObjectCodeMap &ir_to_object_code;
  ObjectCodeVisitor(const IRMemoryAddressTable &, bits::span<quint8>, std::vector<void *> &, IR2ObjectCodeMap &);
  void visit(const EmptyLine *) override;
  void visit(const CommentLine *) override;
  void visit(const MonadicInstruction *) override;
  void visit(const DyadicInstruction *) override;
  void visit(const DotAlign *) override;
  void visit(const DotLiteral *) override;
  void visit(const DotBlock *) override;
  void visit(const DotEquate *) override;
  void visit(const DotSection *) override;
  void visit(const DotAnnotate *) override;
  void visit(const DotOrg *) override;
};

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
} // namespace pepp::tc::ir
