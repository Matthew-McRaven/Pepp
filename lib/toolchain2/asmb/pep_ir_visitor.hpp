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

} // namespace pepp::tc::ir
