/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "core/langs/asmb_riscv/pseudo_instructions.hpp"
#include <memory>
#include <vector>
#include "core/compile/macro/macro_registry.hpp"

namespace {
using pepp::tc::MacroDefinition;

// Assuming XLEN==32.
// Pseudo-instructions which always expand to the same instruction sequence.

// Sign extend byte: sext.b rd, rs -> slli rd, rs, XLEN - 8; srai rd, rd, XLEN - 8
MacroDefinition sext_b() {
  return {"sext.b",
          {{.name = "rd"}, {.name = "rs"}},
          R"(slli \rd, \rs, 24
srai \rd, \rd, 24
)",
          false};
}

// Sign extend halfword: sext.h rd, rs -> slli rd, rs, XLEN - 16; srai rd, rd, XLEN - 16
MacroDefinition sext_h() {
  return {"sext.h",
          {{.name = "rd"}, {.name = "rs"}},
          R"(slli \rd, \rs, 16
srai \rd, \rd, 16
)",
          false};
}

// Zero extend halfword: zext.h rd, rs -> slli rd, rs, XLEN - 16; srli rd, rd, XLEN - 16
MacroDefinition zext_h() {
  return {"zext.h",
          {{.name = "rd"}, {.name = "rs"}},
          R"(slli \rd, \rs, 16
srli \rd, \rd, 16
)",
          false};
}

// Call far-away subroutine: call rd, symbol -> auipc rd, %pcrel_hi(symbol); jalr rd, %pcrel_lo(<the auipc>)(rd)
MacroDefinition call_rd() {
  return {"call",
          {{.name = "rd"}, {.name = "symbol"}},
          R"(.Lpcrel_hi\@: auipc \rd, %pcrel_hi(\symbol)
jalr \rd, %pcrel_lo(.Lpcrel_hi\@)(\rd)
)",
          false};
}

// Call far-away subroutine: call symbol -> call ra, symbol
MacroDefinition call() {
  return {"call",
          {{.name = "symbol"}},
          R"(.Lpcrel_hi\@: auipc ra, %pcrel_hi(\symbol)
jalr ra, %pcrel_lo(.Lpcrel_hi\@)(ra)
)",
          false};
}

// Tail call far-away subroutine: tail symbol -> auipc t1, %pcrel_hi(symbol); jalr zero, %pcrel_lo(<the auipc>)(t1)
MacroDefinition tail() {
  return {"tail",
          {{.name = "symbol"}},
          R"(.Lpcrel_hi\@: auipc t1, %pcrel_hi(\symbol)
jalr zero, %pcrel_lo(.Lpcrel_hi\@)(t1)
)",
          false};
}

// Jump to far-away routine: jump symbol, rt -> auipc rt, %pcrel_hi(symbol); jalr zero, %pcrel_lo(<the auipc>)(rt)
MacroDefinition jump() {
  return {"jump",
          {{.name = "symbol"}, {.name = "rt"}},
          R"(.Lpcrel_hi\@: auipc \rt, %pcrel_hi(\symbol)
jalr zero, %pcrel_lo(.Lpcrel_hi\@)(\rt)
)",
          false};
}

const std::vector<MacroDefinition> &definitions() {
  static const std::vector<MacroDefinition> ret{sext_b(), sext_h(), zext_h(), call(), call_rd(), tail(), jump()};
  return ret;
}
} // namespace

void pepp::tc::riscv_insert_pseudo_instructions(MacroRegistry &registry) {
  // Fails, keeping the existing macro, if the name is already defined.
  for (const auto &definition : definitions()) (void)registry.insert(std::make_shared<MacroDefinition>(definition));
}
