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
#include "enums/isa/rv_types.hpp"

extern const riscv::Instruction<uint32_t> instr32i_FLW;
extern const riscv::Instruction<uint32_t> instr32i_FLD;
extern const riscv::Instruction<uint32_t> instr32i_FSW;
extern const riscv::Instruction<uint32_t> instr32i_FSD;
extern const riscv::Instruction<uint32_t> instr32i_FMADD;
extern const riscv::Instruction<uint32_t> instr32i_FMSUB;
extern const riscv::Instruction<uint32_t> instr32i_FNMADD;
extern const riscv::Instruction<uint32_t> instr32i_FNMSUB;
extern const riscv::Instruction<uint32_t> instr32i_FADD;
extern const riscv::Instruction<uint32_t> instr32i_FSUB;
extern const riscv::Instruction<uint32_t> instr32i_FMUL;
extern const riscv::Instruction<uint32_t> instr32i_FDIV;
extern const riscv::Instruction<uint32_t> instr32i_FSQRT;
extern const riscv::Instruction<uint32_t> instr32i_FMIN_FMAX;
extern const riscv::Instruction<uint32_t> instr32i_FEQ_FLT_FLE;
extern const riscv::Instruction<uint32_t> instr32i_FCVT_SD_DS;
extern const riscv::Instruction<uint32_t> instr32i_FCVT_W_SD;
extern const riscv::Instruction<uint32_t> instr32i_FCVT_SD_W;
extern const riscv::Instruction<uint32_t> instr32i_FSGNJ_NX;
extern const riscv::Instruction<uint32_t> instr32i_FCLASS;
extern const riscv::Instruction<uint32_t> instr32i_FMV_X_W;
extern const riscv::Instruction<uint32_t> instr32i_FMV_W_X;

extern const riscv::Instruction<uint64_t> instr64i_FLW;
extern const riscv::Instruction<uint64_t> instr64i_FLD;
extern const riscv::Instruction<uint64_t> instr64i_FSW;
extern const riscv::Instruction<uint64_t> instr64i_FSD;
extern const riscv::Instruction<uint64_t> instr64i_FMADD;
extern const riscv::Instruction<uint64_t> instr64i_FMSUB;
extern const riscv::Instruction<uint64_t> instr64i_FNMADD;
extern const riscv::Instruction<uint64_t> instr64i_FNMSUB;
extern const riscv::Instruction<uint64_t> instr64i_FADD;
extern const riscv::Instruction<uint64_t> instr64i_FSUB;
extern const riscv::Instruction<uint64_t> instr64i_FMUL;
extern const riscv::Instruction<uint64_t> instr64i_FDIV;
extern const riscv::Instruction<uint64_t> instr64i_FSQRT;
extern const riscv::Instruction<uint64_t> instr64i_FMIN_FMAX;
extern const riscv::Instruction<uint64_t> instr64i_FEQ_FLT_FLE;
extern const riscv::Instruction<uint64_t> instr64i_FCVT_SD_DS;
extern const riscv::Instruction<uint64_t> instr64i_FCVT_W_SD;
extern const riscv::Instruction<uint64_t> instr64i_FCVT_SD_W;
extern const riscv::Instruction<uint64_t> instr64i_FSGNJ_NX;
extern const riscv::Instruction<uint64_t> instr64i_FCLASS;
extern const riscv::Instruction<uint64_t> instr64i_FMV_X_W;
extern const riscv::Instruction<uint64_t> instr64i_FMV_W_X;
