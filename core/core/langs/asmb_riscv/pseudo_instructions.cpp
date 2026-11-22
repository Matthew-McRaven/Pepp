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
#include <string>
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

// Returns an auipc+jalr pair which calls symbol through rt and links rd. In an ET_REL file, the auipc's
// R_RISCV_CALL_PLT relocates both instructions, so the jalr holds 0. Otherwise, the assembler resolves both.
std::string far_call(pepp::bts::ElfFileType output_type, const std::string &rt, const std::string &rd) {
  if (output_type == pepp::bts::ElfFileType::ET_REL)
    return "auipc " + rt + ", %call_plt(\\symbol)\njalr " + rd + ", 0(" + rt + ")\n";
  return ".Lpcrel_hi\\@: auipc " + rt + ", %pcrel_hi(\\symbol)\njalr " + rd + ", %pcrel_lo(.Lpcrel_hi\\@)(" + rt +
         ")\n";
}

// Call far-away subroutine: call rd, symbol
MacroDefinition call_rd(pepp::bts::ElfFileType output_type) {
  return {"call", {{.name = "rd"}, {.name = "symbol"}}, far_call(output_type, "\\rd", "\\rd"), false};
}

// Call far-away subroutine: call symbol -> call ra, symbol
MacroDefinition call(pepp::bts::ElfFileType output_type) {
  return {"call", {{.name = "symbol"}}, far_call(output_type, "ra", "ra"), false};
}

// Tail call far-away subroutine: tail symbol -> jump symbol, t1
MacroDefinition tail(pepp::bts::ElfFileType output_type) {
  return {"tail", {{.name = "symbol"}}, far_call(output_type, "t1", "zero"), false};
}

// Jump to far-away routine: jump symbol, rt
MacroDefinition jump(pepp::bts::ElfFileType output_type) {
  return {"jump", {{.name = "symbol"}, {.name = "rt"}}, far_call(output_type, "\\rt", "zero"), false};
}

// Load local address: lla rd, symbol -> auipc rd, %pcrel_hi(symbol); addi rd, rd, %pcrel_lo(<the auipc>)
// Without .option pic, la is equivalent.
MacroDefinition load_address(const std::string &mnemonic) {
  return {mnemonic,
          {{.name = "rd"}, {.name = "symbol"}},
          R"(.Lpcrel_hi\@: auipc \rd, %pcrel_hi(\symbol)
addi \rd, \rd, %pcrel_lo(.Lpcrel_hi\@)
)",
          false};
}

// Load immediate. To quote the official RISC-V assembler's programming guide on the implementation:
// "Myriad sequences". In our implementation, if the imm fits in 12 bits, we use addi x0, imm. If the imm is a multiple
// of 4096, we use lui. Otherwise, we use lui+addi.
MacroDefinition li() {
  return {"li",
          {{.name = "rd"}, {.name = "imm"}},
          R"(.IF !%constant(\imm)
.ERROR "li requires a constant defined before use"
.ELSEIF %bitwidth(\imm) > 32
.ERROR "li requires a value which fits in 32 bits"
.ELSEIF (((\imm) + 0x800) & 0xFFFFF000) == 0
addi \rd, x0, \imm
.ELSEIF ((\imm) & 0xFFF) == 0
lui \rd, %hi(\imm)
.ELSE
lui \rd, %hi(\imm)
addi \rd, \rd, %lo(\imm)
.ENDIF
)",
          false,
          false};
}

// Load global: l{b|h|w|bu|hu} rd, symbol -> auipc rd, %pcrel_hi(symbol); l{b|h|w|bu|hu} rd, %pcrel_lo(<the auipc>)(rd)
// Shares its name with the load instruction, only differing by arity.
MacroDefinition load_global(const std::string &mnemonic) {
  return {mnemonic,
          {{.name = "rd"}, {.name = "symbol"}},
          ".Lpcrel_hi\\@: auipc \\rd, %pcrel_hi(\\symbol)\n" + mnemonic + " \\rd, %pcrel_lo(.Lpcrel_hi\\@)(\\rd)\n",
          false};
}

// Store global: s{b|h|w} rs, symbol, rt -> auipc rt, %pcrel_hi(symbol); s{b|h|w} rs, %pcrel_lo(<the auipc>)(rt)
// Shares its mnemonic with the store instruction, only differing by arity.
MacroDefinition store_global(const std::string &mnemonic) {
  return {mnemonic,
          {{.name = "rs"}, {.name = "symbol"}, {.name = "rt"}},
          ".Lpcrel_hi\\@: auipc \\rt, %pcrel_hi(\\symbol)\n" + mnemonic + " \\rs, %pcrel_lo(.Lpcrel_hi\\@)(\\rt)\n",
          false};
}

std::vector<MacroDefinition> definitions(pepp::bts::ElfFileType output_type) {
  return {
      sext_b(),           sext_h(),           zext_h(),           call(output_type), call_rd(output_type),
      tail(output_type),  jump(output_type),  li(),               load_address("la"), load_address("lla"),
      load_global("lb"),  load_global("lh"),  load_global("lw"),  load_global("lbu"), load_global("lhu"),
      store_global("sb"), store_global("sh"), store_global("sw"),
  };
}
} // namespace

void pepp::tc::riscv_insert_pseudo_instructions(MacroRegistry &registry, bts::ElfFileType output_type) {
  // Fails, keeping the existing macro, if the name is already defined.
  for (const auto &definition : definitions(output_type))
    (void)registry.insert(std::make_shared<MacroDefinition>(definition));
}
