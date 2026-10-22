/*
 * Copyright (c) 2025-2026 J. Stanley Warford, Matthew McRaven
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

#pragma once

namespace pepp::tc {
struct LinearIR;
struct EmptyLine;
struct CommentLine;
struct SymbolLine;
struct MonadicInstruction;
struct DyadicInstruction;
struct DotAlign;
struct DotLiteral;
struct DotBlock;
struct DotEquate;
struct DotSection;
struct DotAnnotate;
struct DotOrg;
struct DotConditional;
struct InlineMacroDefinition;
struct MacroInstantiation;
struct PepIRVisitor {
  virtual void visit(const EmptyLine *) = 0;
  virtual void visit(const CommentLine *) = 0;
  virtual void visit(const SymbolLine *) = 0;
  virtual void visit(const MonadicInstruction *) = 0;
  virtual void visit(const DyadicInstruction *) = 0;
  virtual void visit(const DotAlign *) = 0;
  virtual void visit(const DotLiteral *) = 0;
  virtual void visit(const DotBlock *) = 0;
  virtual void visit(const DotEquate *) = 0;
  virtual void visit(const DotSection *) = 0;
  virtual void visit(const DotAnnotate *) = 0;
  virtual void visit(const DotOrg *) = 0;
  virtual void visit(const DotConditional *) = 0;
  virtual void visit(const InlineMacroDefinition *) = 0;
  virtual void visit(const MacroInstantiation *) = 0;
  void accept(const LinearIR *line);
};
void accept(PepIRVisitor &visitor, const LinearIR *line);

} // namespace pepp::tc
