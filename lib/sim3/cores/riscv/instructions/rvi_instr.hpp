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
#include <cstdint>
#include "../../../../core/isa/riscv/rv_types.hpp"

extern const riscv::Instruction<uint32_t> instr32i_NOP;
extern const riscv::Instruction<uint32_t> instr32i_UNIMPLEMENTED;
extern const riscv::Instruction<uint32_t> instr32i_ILLEGAL;
extern const riscv::Instruction<uint32_t> instr32i_LOAD_I8;
extern const riscv::Instruction<uint32_t> instr32i_LOAD_I16;
extern const riscv::Instruction<uint32_t> instr32i_LOAD_I32;
extern const riscv::Instruction<uint32_t> instr32i_LOAD_I64;
extern const riscv::Instruction<uint32_t> instr32i_LOAD_U8;
extern const riscv::Instruction<uint32_t> instr32i_LOAD_U16;
extern const riscv::Instruction<uint32_t> instr32i_LOAD_U32;
extern const riscv::Instruction<uint32_t> instr32i_LOAD_U64;
extern const riscv::Instruction<uint32_t> instr32i_LOAD_X_DUMMY;
extern const riscv::Instruction<uint32_t> instr32i_STORE_I8_IMM;
extern const riscv::Instruction<uint32_t> instr32i_STORE_I8;
extern const riscv::Instruction<uint32_t> instr32i_STORE_I16_IMM;
extern const riscv::Instruction<uint32_t> instr32i_STORE_I32_IMM;
extern const riscv::Instruction<uint32_t> instr32i_STORE_I64_IMM;
extern const riscv::Instruction<uint32_t> instr32i_BRANCH_EQ;
extern const riscv::Instruction<uint32_t> instr32i_BRANCH_NE;
extern const riscv::Instruction<uint32_t> instr32i_BRANCH_LT;
extern const riscv::Instruction<uint32_t> instr32i_BRANCH_GE;
extern const riscv::Instruction<uint32_t> instr32i_BRANCH_LTU;
extern const riscv::Instruction<uint32_t> instr32i_BRANCH_GEU;
extern const riscv::Instruction<uint32_t> instr32i_JALR;
extern const riscv::Instruction<uint32_t> instr32i_JAL;
extern const riscv::Instruction<uint32_t> instr32i_JMPI;
extern const riscv::Instruction<uint32_t> instr32i_OP_IMM;
extern const riscv::Instruction<uint32_t> instr32i_OP_IMM_ADDI;
extern const riscv::Instruction<uint32_t> instr32i_OP_IMM_LI;
extern const riscv::Instruction<uint32_t> instr32i_OP_MV;
extern const riscv::Instruction<uint32_t> instr32i_OP_IMM_SLLI;
extern const riscv::Instruction<uint32_t> instr32i_OP_IMM_SRLI;
extern const riscv::Instruction<uint32_t> instr32i_OP_IMM_ANDI;
extern const riscv::Instruction<uint32_t> instr32i_OP;
extern const riscv::Instruction<uint32_t> instr32i_OP_ADD;
extern const riscv::Instruction<uint32_t> instr32i_OP_SUB;
extern const riscv::Instruction<uint32_t> instr32i_SYSTEM;
extern const riscv::Instruction<uint32_t> instr32i_SYSCALL;
extern const riscv::Instruction<uint32_t> instr32i_WFI;
extern const riscv::Instruction<uint32_t> instr32i_LUI;
extern const riscv::Instruction<uint32_t> instr32i_AUIPC;
extern const riscv::Instruction<uint32_t> instr32i_OP_IMM32_ADDIW;
extern const riscv::Instruction<uint32_t> instr32i_OP_IMM32_SLLIW;
extern const riscv::Instruction<uint32_t> instr32i_OP_IMM32_SRLIW;
extern const riscv::Instruction<uint32_t> instr32i_OP_IMM32_SRAIW;
extern const riscv::Instruction<uint32_t> instr32i_OP_IMM32_SLLI_UW;
extern const riscv::Instruction<uint32_t> instr32i_OP_IMM32;
extern const riscv::Instruction<uint32_t> instr32i_OP32;
extern const riscv::Instruction<uint32_t> instr32i_OP32_ADDW;
extern const riscv::Instruction<uint32_t> instr32i_FENCE;

extern const riscv::Instruction<uint64_t> instr64i_NOP;
extern const riscv::Instruction<uint64_t> instr64i_UNIMPLEMENTED;
extern const riscv::Instruction<uint64_t> instr64i_ILLEGAL;
extern const riscv::Instruction<uint64_t> instr64i_LOAD_I8;
extern const riscv::Instruction<uint64_t> instr64i_LOAD_I16;
extern const riscv::Instruction<uint64_t> instr64i_LOAD_I32;
extern const riscv::Instruction<uint64_t> instr64i_LOAD_I64;
extern const riscv::Instruction<uint64_t> instr64i_LOAD_U8;
extern const riscv::Instruction<uint64_t> instr64i_LOAD_U16;
extern const riscv::Instruction<uint64_t> instr64i_LOAD_U32;
extern const riscv::Instruction<uint64_t> instr64i_LOAD_U64;
extern const riscv::Instruction<uint64_t> instr64i_LOAD_X_DUMMY;
extern const riscv::Instruction<uint64_t> instr64i_STORE_I8_IMM;
extern const riscv::Instruction<uint64_t> instr64i_STORE_I8;
extern const riscv::Instruction<uint64_t> instr64i_STORE_I16_IMM;
extern const riscv::Instruction<uint64_t> instr64i_STORE_I32_IMM;
extern const riscv::Instruction<uint64_t> instr64i_STORE_I64_IMM;
extern const riscv::Instruction<uint64_t> instr64i_BRANCH_EQ;
extern const riscv::Instruction<uint64_t> instr64i_BRANCH_NE;
extern const riscv::Instruction<uint64_t> instr64i_BRANCH_LT;
extern const riscv::Instruction<uint64_t> instr64i_BRANCH_GE;
extern const riscv::Instruction<uint64_t> instr64i_BRANCH_LTU;
extern const riscv::Instruction<uint64_t> instr64i_BRANCH_GEU;
extern const riscv::Instruction<uint64_t> instr64i_JALR;
extern const riscv::Instruction<uint64_t> instr64i_JAL;
extern const riscv::Instruction<uint64_t> instr64i_JMPI;
extern const riscv::Instruction<uint64_t> instr64i_OP_IMM;
extern const riscv::Instruction<uint64_t> instr64i_OP_IMM_ADDI;
extern const riscv::Instruction<uint64_t> instr64i_OP_IMM_LI;
extern const riscv::Instruction<uint64_t> instr64i_OP_MV;
extern const riscv::Instruction<uint64_t> instr64i_OP_IMM_SLLI;
extern const riscv::Instruction<uint64_t> instr64i_OP_IMM_SRLI;
extern const riscv::Instruction<uint64_t> instr64i_OP_IMM_ANDI;
extern const riscv::Instruction<uint64_t> instr64i_OP;
extern const riscv::Instruction<uint64_t> instr64i_OP_ADD;
extern const riscv::Instruction<uint64_t> instr64i_OP_SUB;
extern const riscv::Instruction<uint64_t> instr64i_SYSTEM;
extern const riscv::Instruction<uint64_t> instr64i_SYSCALL;
extern const riscv::Instruction<uint64_t> instr64i_WFI;
extern const riscv::Instruction<uint64_t> instr64i_LUI;
extern const riscv::Instruction<uint64_t> instr64i_AUIPC;
extern const riscv::Instruction<uint64_t> instr64i_OP_IMM32_ADDIW;
extern const riscv::Instruction<uint64_t> instr64i_OP_IMM32_SLLIW;
extern const riscv::Instruction<uint64_t> instr64i_OP_IMM32_SRLIW;
extern const riscv::Instruction<uint64_t> instr64i_OP_IMM32_SRAIW;
extern const riscv::Instruction<uint64_t> instr64i_OP_IMM32_SLLI_UW;
extern const riscv::Instruction<uint64_t> instr64i_OP_IMM32;
extern const riscv::Instruction<uint64_t> instr64i_OP32;
extern const riscv::Instruction<uint64_t> instr64i_OP32_ADDW;
extern const riscv::Instruction<uint64_t> instr64i_FENCE;
