#pragma once

namespace pepp::tc {
struct LinearIR;
struct EmptyLine;
struct CommentLine;
struct RTypeIR;
struct ITypeIR;
struct STypeIR;
struct BTypeIR;
struct UTypeIR;
struct JTypeIR;
struct DotAlign;
struct DotLiteral;
struct DotBlock;
struct DotEquate;
struct DotSection;
struct DotOrg;
struct RISCVIRVisitor {
  virtual void visit(const EmptyLine *) = 0;
  virtual void visit(const CommentLine *) = 0;
  virtual void visit(const RTypeIR *) = 0;
  virtual void visit(const ITypeIR *) = 0;
  virtual void visit(const STypeIR *) = 0;
  virtual void visit(const BTypeIR *) = 0;
  virtual void visit(const UTypeIR *) = 0;
  virtual void visit(const JTypeIR *) = 0;
  virtual void visit(const DotAlign *) = 0;
  virtual void visit(const DotLiteral *) = 0;
  virtual void visit(const DotBlock *) = 0;
  virtual void visit(const DotEquate *) = 0;
  virtual void visit(const DotSection *) = 0;
  virtual void visit(const DotOrg *) = 0;
  void accept(const LinearIR *line);
};
void accept(RISCVIRVisitor &visitor, const LinearIR *line);

} // namespace pepp::tc
