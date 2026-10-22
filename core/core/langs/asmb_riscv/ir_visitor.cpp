/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ir_visitor.hpp"
#include "core/compile/ir_linear/line_base.hpp"
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_linear/line_macro.hpp"
#include "core/compile/ir_linear/line_symbol.hpp"
#include "core/langs/asmb_riscv/ir_lines.hpp"

void pepp::tc::accept(RISCVIRVisitor &visitor, const LinearIR *line) {
  using Type = LinearIRType;
  switch (line->type()) {
  case EmptyLine::TYPE: visitor.visit(static_cast<const EmptyLine *>(line)); break;
  case CommentLine::TYPE: visitor.visit(static_cast<const CommentLine *>(line)); break;
  case SymbolLine::TYPE: visitor.visit(static_cast<const SymbolLine *>(line)); break;
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
  case DotConditional::TYPE: visitor.visit(static_cast<const DotConditional *>(line)); break;
  case InlineMacroDefinition::TYPE: visitor.visit(static_cast<const InlineMacroDefinition *>(line)); break;
  case MacroInstantiation::TYPE: visitor.visit(static_cast<const MacroInstantiation *>(line)); break;
  default: throw std::logic_error("Unknown IR line type");
  }
}

void pepp::tc::RISCVIRVisitor::accept(const LinearIR *line) { return pepp::tc::accept(*this, line); }
