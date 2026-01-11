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
#include "bts/isa/rv_types.hpp"

extern const riscv::Instruction<uint32_t> instr32i_C0_ADDI4SPN;
extern const riscv::Instruction<uint32_t> instr32i_C0_REG_FLD;
extern const riscv::Instruction<uint32_t> instr32i_C0_REG_LW;
extern const riscv::Instruction<uint32_t> instr32i_C0_REG_LD;
extern const riscv::Instruction<uint32_t> instr32i_C0_REG_FLW;
extern const riscv::Instruction<uint32_t> instr32i_C0_REG_FSD;
extern const riscv::Instruction<uint32_t> instr32i_C0_REG_SW;
extern const riscv::Instruction<uint32_t> instr32i_C0_REG_SD;
extern const riscv::Instruction<uint32_t> instr32i_C0_REG_FSW;
extern const riscv::Instruction<uint32_t> instr32i_C1_ADDI;
extern const riscv::Instruction<uint32_t> instr32i_C1_JAL;
extern const riscv::Instruction<uint32_t> instr32i_C1_ADDIW;
extern const riscv::Instruction<uint32_t> instr32i_C1_LI;
extern const riscv::Instruction<uint32_t> instr32i_C1_ADDI16SP;
extern const riscv::Instruction<uint32_t> instr32i_C1_LUI;
extern const riscv::Instruction<uint32_t> instr32i_C1_ALU_OPS;
extern const riscv::Instruction<uint32_t> instr32i_C1_JUMP;
extern const riscv::Instruction<uint32_t> instr32i_C1_BEQZ;
extern const riscv::Instruction<uint32_t> instr32i_C1_BNEZ;
extern const riscv::Instruction<uint32_t> instr32i_C2_SLLI;
extern const riscv::Instruction<uint32_t> instr32i_C2_FLDSP;
extern const riscv::Instruction<uint32_t> instr32i_C2_LWSP;
extern const riscv::Instruction<uint32_t> instr32i_C2_LDSP;
extern const riscv::Instruction<uint32_t> instr32i_C2_FLWSP;
extern const riscv::Instruction<uint32_t> instr32i_C2_FSDSP;
extern const riscv::Instruction<uint32_t> instr32i_C2_SWSP;
extern const riscv::Instruction<uint32_t> instr32i_C2_SDSP;
extern const riscv::Instruction<uint32_t> instr32i_C2_FSWSP;
extern const riscv::Instruction<uint32_t> instr32i_C2_JR;
extern const riscv::Instruction<uint32_t> instr32i_C2_JALR;
extern const riscv::Instruction<uint32_t> instr32i_C2_MV;
extern const riscv::Instruction<uint32_t> instr32i_C2_ADD;
extern const riscv::Instruction<uint32_t> instr32i_C2_EBREAK;

extern const riscv::Instruction<uint64_t> instr64i_C0_ADDI4SPN;
extern const riscv::Instruction<uint64_t> instr64i_C0_REG_FLD;
extern const riscv::Instruction<uint64_t> instr64i_C0_REG_LW;
extern const riscv::Instruction<uint64_t> instr64i_C0_REG_LD;
extern const riscv::Instruction<uint64_t> instr64i_C0_REG_FLW;
extern const riscv::Instruction<uint64_t> instr64i_C0_REG_FSD;
extern const riscv::Instruction<uint64_t> instr64i_C0_REG_SW;
extern const riscv::Instruction<uint64_t> instr64i_C0_REG_SD;
extern const riscv::Instruction<uint64_t> instr64i_C0_REG_FSW;
extern const riscv::Instruction<uint64_t> instr64i_C1_ADDI;
extern const riscv::Instruction<uint64_t> instr64i_C1_JAL;
extern const riscv::Instruction<uint64_t> instr64i_C1_ADDIW;
extern const riscv::Instruction<uint64_t> instr64i_C1_LI;
extern const riscv::Instruction<uint64_t> instr64i_C1_ADDI16SP;
extern const riscv::Instruction<uint64_t> instr64i_C1_LUI;
extern const riscv::Instruction<uint64_t> instr64i_C1_ALU_OPS;
extern const riscv::Instruction<uint64_t> instr64i_C1_JUMP;
extern const riscv::Instruction<uint64_t> instr64i_C1_BEQZ;
extern const riscv::Instruction<uint64_t> instr64i_C1_BNEZ;
extern const riscv::Instruction<uint64_t> instr64i_C2_SLLI;
extern const riscv::Instruction<uint64_t> instr64i_C2_FLDSP;
extern const riscv::Instruction<uint64_t> instr64i_C2_LWSP;
extern const riscv::Instruction<uint64_t> instr64i_C2_LDSP;
extern const riscv::Instruction<uint64_t> instr64i_C2_FLWSP;
extern const riscv::Instruction<uint64_t> instr64i_C2_FSDSP;
extern const riscv::Instruction<uint64_t> instr64i_C2_SWSP;
extern const riscv::Instruction<uint64_t> instr64i_C2_SDSP;
extern const riscv::Instruction<uint64_t> instr64i_C2_FSWSP;
extern const riscv::Instruction<uint64_t> instr64i_C2_JR;
extern const riscv::Instruction<uint64_t> instr64i_C2_JALR;
extern const riscv::Instruction<uint64_t> instr64i_C2_MV;
extern const riscv::Instruction<uint64_t> instr64i_C2_ADD;
extern const riscv::Instruction<uint64_t> instr64i_C2_EBREAK;
