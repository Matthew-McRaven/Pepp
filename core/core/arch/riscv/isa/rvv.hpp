/*
 * Copyright (c) 2025-2026 J. Stanley Warford, Matthew McRaven
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
 *
 * Copyright (c) 2024, Alf-André Walla
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the BSD 3-clause license
 * along with this program. If not, see
 * <https://opensource.org/license/bsd-3-clause>
 */
#pragma once
#include "rvi.hpp"

namespace riscv {

// Vector Load
struct InstructionVL {
  uint32_t opcode : 7;
  uint32_t vd : 5;
  uint32_t width : 3;
  uint32_t rs1 : 5;
  uint32_t lumop : 5;
  uint32_t vm : 1;
  uint32_t mop : 2;
  uint32_t mew : 1;
  uint32_t nf : 3;
};
static_assert(sizeof(InstructionVL) == 4, "VL instruction must be 32 bits");

struct InstructionVLS {
  uint32_t opcode : 7;
  uint32_t vd : 5;
  uint32_t width : 3;
  uint32_t rs1 : 5;
  uint32_t rs2 : 5;
  uint32_t vm : 1;
  uint32_t mop : 2;
  uint32_t mew : 1;
  uint32_t nf : 3;
};
static_assert(sizeof(InstructionVLS) == 4, "VLS instruction must be 32 bits");

struct InstructionVLX {
  uint32_t opcode : 7;
  uint32_t vd : 5;
  uint32_t width : 3;
  uint32_t rs1 : 5;
  uint32_t vs2 : 5;
  uint32_t vm : 1;
  uint32_t mop : 2;
  uint32_t mew : 1;
  uint32_t nf : 3;
};
static_assert(sizeof(InstructionVLX) == 4, "VLX instruction must be 32 bits");

// Vector Store
struct InstructionVS {
  uint32_t opcode : 7;
  uint32_t vs3 : 5;
  uint32_t width : 3;
  uint32_t rs1 : 5;
  uint32_t sumop : 5;
  uint32_t vm : 1;
  uint32_t mop : 2;
  uint32_t mew : 1;
  uint32_t nf : 3;
};
static_assert(sizeof(InstructionVS) == 4, "VS instruction must be 32 bits");

struct InstructionVSS {
  uint32_t opcode : 7;
  uint32_t vs3 : 5;
  uint32_t width : 3;
  uint32_t rs1 : 5;
  uint32_t rs2 : 5;
  uint32_t vm : 1;
  uint32_t mop : 2;
  uint32_t mew : 1;
  uint32_t nf : 3;
};
static_assert(sizeof(InstructionVS) == 4, "VS instruction must be 32 bits");

struct InstructionVSX {
  uint32_t opcode : 7;
  uint32_t vs3 : 5;
  uint32_t width : 3;
  uint32_t rs1 : 5;
  uint32_t vs2 : 5;
  uint32_t vm : 1;
  uint32_t mop : 2;
  uint32_t mew : 1;
  uint32_t nf : 3;
};
static_assert(sizeof(InstructionVS) == 4, "VS instruction must be 32 bits");

// Vector Configuration
struct InstructionVLI {
  uint32_t opcode : 7;
  uint32_t rd : 5;
  uint32_t ones : 3;
  uint32_t rs1 : 5;
  uint32_t zimm : 12;
}; // 0, 1 && 0, 0
static_assert(sizeof(InstructionVLI) == 4, "VLI instruction must be 32 bits");

struct InstructionIVLI {
  uint32_t opcode : 7;
  uint32_t rd : 5;
  uint32_t ones : 3;
  uint32_t uimm : 5;
  uint32_t zimm : 12;
}; // 1, 1
static_assert(sizeof(InstructionVLI) == 4, "VLI instruction must be 32 bits");

struct InstructionVSETVL {
  uint32_t opcode : 7;
  uint32_t rd : 5;
  uint32_t ones : 3;
  uint32_t rs1 : 5;
  uint32_t rs2 : 5;
  uint32_t zimm : 7;
}; // 1, 0
static_assert(sizeof(InstructionVSETVL) == 4, "VSETVL instruction must be 32 bits");

struct InstructionOPVV {
  uint32_t opcode : 7;
  uint32_t vd : 5;
  uint32_t funct3 : 3;
  uint32_t vs1 : 5;
  uint32_t vs2 : 5;
  uint32_t vm : 1;
  uint32_t funct6 : 6;
};
static_assert(sizeof(InstructionOPVV) == 4, "OPVV instruction must be 32 bits");

struct InstructionOPVI {
  uint32_t opcode : 7;
  uint32_t vd : 5;
  uint32_t funct3 : 3;
  uint32_t imm : 5;
  uint32_t vs2 : 5;
  uint32_t vm : 1;
  uint32_t funct6 : 6;
};
static_assert(sizeof(InstructionOPVI) == 4, "OPVI instruction must be 32 bits");

union rv32v_instruction {
  InstructionVL VL;
  InstructionVLS VLS;
  InstructionVLX VLX;
  InstructionVS VS;
  InstructionVSS VSS;
  InstructionVSX VSX;
  InstructionVLI VLI;
  InstructionIVLI IVLI;
  InstructionVSETVL VSETVL;
  InstructionOPVV OPVV;
  InstructionOPVI OPVI;
  rv32v_instruction(const rv32i_instruction &i) : whole(i.whole) {}
  uint32_t whole;
};
static_assert(sizeof(rv32v_instruction) == 4, "Instructions are 32-bits");

} // namespace riscv
