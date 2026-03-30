#include "core/langs/asmb_pep/ir_visitor.hpp"
#include "core/compile/ir_linear/line_base.hpp"
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/langs/asmb_pep/ir_lines.hpp"

void pepp::tc::accept(PepIRVisitor &visitor, const LinearIR *line) {
  using Type = LinearIRType;
  switch (line->type()) {
  case EmptyLine::TYPE: visitor.visit(static_cast<const EmptyLine *>(line)); break;
  case CommentLine::TYPE: visitor.visit(static_cast<const CommentLine *>(line)); break;
  case MonadicInstruction::TYPE: visitor.visit(static_cast<const MonadicInstruction *>(line)); break;
  case DyadicInstruction::TYPE: visitor.visit(static_cast<const DyadicInstruction *>(line)); break;
  case DotAlign::TYPE: visitor.visit(static_cast<const DotAlign *>(line)); break;
  case DotLiteral::TYPE: visitor.visit(static_cast<const DotLiteral *>(line)); break;
  case DotBlock::TYPE: visitor.visit(static_cast<const DotBlock *>(line)); break;
  case DotEquate::TYPE: visitor.visit(static_cast<const DotEquate *>(line)); break;
  case DotSection::TYPE: visitor.visit(static_cast<const DotSection *>(line)); break;
  case DotAnnotate::TYPE: visitor.visit(static_cast<const DotAnnotate *>(line)); break;
  case DotOrg::TYPE: visitor.visit(static_cast<const DotOrg *>(line)); break;
  default: throw std::logic_error("Unknown IR line type");
  }
}
