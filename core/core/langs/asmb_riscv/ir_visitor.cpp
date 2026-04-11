#include "ir_visitor.hpp"
#include "core/compile/ir_linear/line_base.hpp"
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/langs/asmb_riscv/ir_lines.hpp"

void pepp::tc::accept(RISCVIRVisitor &visitor, const LinearIR *line) {
  using Type = LinearIRType;
  switch (line->type()) {
  case EmptyLine::TYPE: visitor.visit(static_cast<const EmptyLine *>(line)); break;
  case CommentLine::TYPE: visitor.visit(static_cast<const CommentLine *>(line)); break;
  case RTypeIR::TYPE: visitor.visit(static_cast<const RTypeIR *>(line)); break;
  case ITypeIR::TYPE: visitor.visit(static_cast<const ITypeIR *>(line)); break;
  case STypeIR::TYPE: visitor.visit(static_cast<const STypeIR *>(line)); break;
  case BTypeIR::TYPE: visitor.visit(static_cast<const BTypeIR *>(line)); break;
  case UTypeIR::TYPE: visitor.visit(static_cast<const UTypeIR *>(line)); break;
  case JTypeIR::TYPE: visitor.visit(static_cast<const JTypeIR *>(line)); break;
  case DotAlign::TYPE: visitor.visit(static_cast<const DotAlign *>(line)); break;
  case DotLiteral::TYPE: visitor.visit(static_cast<const DotLiteral *>(line)); break;
  case DotBlock::TYPE: visitor.visit(static_cast<const DotBlock *>(line)); break;
  case DotEquate::TYPE: visitor.visit(static_cast<const DotEquate *>(line)); break;
  case DotSection::TYPE: visitor.visit(static_cast<const DotSection *>(line)); break;
  case DotOrg::TYPE: visitor.visit(static_cast<const DotOrg *>(line)); break;
  default: throw std::logic_error("Unknown IR line type");
  }
}

void pepp::tc::RISCVIRVisitor::accept(const LinearIR *line) { return pepp::tc::accept(*this, line); }
