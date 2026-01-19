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

#define RV32I_LOAD     0b0000011
#define RV32I_STORE    0b0100011
#define RV32I_BRANCH   0b1100011
#define RV32I_JALR     0b1100111
#define RV32I_JAL      0b1101111
#define RV32I_OP_IMM   0b0010011
#define RV32I_OP       0b0110011
#define RV32I_SYSTEM   0b1110011
#define RV32I_LUI      0b0110111
#define RV32I_AUIPC    0b0010111
#define RV32I_FENCE    0b0001111
#define RV64I_OP_IMM32 0b0011011
#define RV64I_OP32     0b0111011
#define RV128I_OP_IMM64 0b1011011
#define RV128I_OP64     0b1111011

#define RV32F_LOAD     0b0000111
#define RV32F_STORE    0b0100111
#define RV32F_FMADD    0b1000011
#define RV32F_FMSUB    0b1000111
#define RV32F_FNMSUB   0b1001011
#define RV32F_FNMADD   0b1001111
#define RV32F_FPFUNC   0b1010011
#define RV32A_ATOMIC   0b0101111

#define RV32F__FADD       0b00000
#define RV32F__FSUB       0b00001
#define RV32F__FMUL       0b00010
#define RV32F__FDIV       0b00011
#define RV32F__FSGNJ_NX   0b00100
#define RV32F__FMIN_MAX   0b00101
#define RV32F__FSQRT      0b01011
#define RV32F__FEQ_LT_LE  0b10100
#define RV32F__FCVT_SD_DS 0b01000
#define RV32F__FCVT_W_SD  0b11000
#define RV32F__FCVT_SD_W  0b11010
#define RV32F__FMV_X_W    0b11100
#define RV32F__FMV_W_X    0b11110

#define RV32V_OP        0b1010111

#define RV32_INSTR_STOP       0x7ff00073
