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
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include "core/integers.h"

namespace riscv {
// clang-format off
enum class XReg:u8 {
  x0=0, x1, x2, x3, x4, x5, x6, x7, x8, x9, 
  x10, x11, x12, x13, x14, x15, x16, x17, x18, x19,
  x20, x21, x22, x23, x24, x25, x26, x27, x28, x29,
  x30, x31
};
enum class ABIReg:u8 {
  zero=0, ra, sp, gp, tp, t0, t1, t2, s0, s1,
  a0, a1, a2, a3, a4, a5, a6, a7, s2, s3,
  s4, s5, s6, s7, s8, s9, s10, s11, t3, t4,
  t5, t6
};
// clang-format on
static const uint32_t REG_ZERO = 0;
static const uint32_t REG_RA = 1;
static const uint32_t REG_SP = 2;
static const uint32_t REG_GP = 3;
static const uint32_t REG_TP = 4;
static const uint32_t REG_T0 = 5;
static const uint32_t REG_T1 = 6;
static const uint32_t REG_RETVAL = 10;
static const uint32_t REG_ARG0 = 10;
static const uint32_t REG_ARG1 = 11;
static const uint32_t REG_ARG2 = 12;
static const uint32_t REG_ARG3 = 13;
static const uint32_t REG_ARG4 = 14;
static const uint32_t REG_ARG5 = 15;
static const uint32_t REG_ARG6 = 16;
static const uint32_t REG_ARG7 = 17;
static const uint32_t REG_ECALL = 17;
/* floating-point helpers */
static const uint32_t REG_FA0 = 10;

struct RISCV {
  static const char *regname(const uint32_t reg) noexcept {
    switch (reg) {
    case 0: return "ZERO";
    case 1: return "RA";
    case 2: return "SP";
    case 3: return "GP";
    case 4: return "TP";
    case 5: return "LR";
    case 6: return "TMP1";
    case 7: return "TMP2";
    case 8: return "SR0";
    case 9: return "SR1";
    case 10: return "A0";
    case 11: return "A1";
    case 12: return "A2";
    case 13: return "A3";
    case 14: return "A4";
    case 15: return "A5";
    case 16: return "A6";
    case 17: return "A7";
    case 18: return "SR2";
    case 19: return "SR3";
    case 20: return "SR4";
    case 21: return "SR5";
    case 22: return "SR6";
    case 23: return "SR7";
    case 24: return "SR8";
    case 25: return "SR9";
    case 26: return "SR10";
    case 27: return "SR11";
    case 28: return "TMP3";
    case 29: return "TMP4";
    case 30: return "TMP5";
    case 31: return "TMP6";
    }
    return "Invalid register";
  }
  static const char *ciname(const uint16_t reg) noexcept { return regname(reg + 0x8); }

  static const char *flpname(const uint32_t reg) noexcept {
    switch (reg) {
    case 0: return "FT0";
    case 1: return "FT1";
    case 2: return "FT2";
    case 3: return "FT3";
    case 4: return "FT4";
    case 5: return "FT5";
    case 6: return "FT6";
    case 7: return "FT7";
    case 8: return "FS0";
    case 9: return "FS1";
    case 10: return "FA0";
    case 11: return "FA1";
    case 12: return "FA2";
    case 13: return "FA3";
    case 14: return "FA4";
    case 15: return "FA5";
    case 16: return "FA6";
    case 17: return "FA7";
    case 18: return "FS2";
    case 19: return "FS3";
    case 20: return "FS4";
    case 21: return "FS5";
    case 22: return "FS6";
    case 23: return "FS7";
    case 24: return "FS8";
    case 25: return "FS9";
    case 26: return "FS10";
    case 27: return "FS11";
    case 28: return "FT8";
    case 29: return "FT9";
    case 30: return "FT10";
    case 31: return "FT11";
    }
    return "Invalid register";
  }
  static inline char flpsize(const uint8_t size) {
    static const char sizechar[4] = {'S', 'D', 'H', 'Q'};
    if (size < 4) return sizechar[size];
    return '?';
  }
  static const char *ciflp(const uint16_t reg) noexcept { return flpname(reg + 0x8); }

  static const char *vecname(const uint32_t reg) noexcept {
    switch (reg) {
    case 0: return "V0";
    case 1: return "V1";
    case 2: return "V2";
    case 3: return "V3";
    case 4: return "V4";
    case 5: return "V5";
    case 6: return "V6";
    case 7: return "V7";
    case 8: return "V8";
    case 9: return "V9";
    case 10: return "V10";
    case 11: return "V11";
    case 12: return "V12";
    case 13: return "V13";
    case 14: return "V14";
    case 15: return "V15";
    case 16: return "V16";
    case 17: return "V17";
    case 18: return "V18";
    case 19: return "V19";
    case 20: return "V20";
    case 21: return "V21";
    case 22: return "V22";
    case 23: return "V23";
    case 24: return "V24";
    case 25: return "V25";
    case 26: return "V26";
    case 27: return "V27";
    case 28: return "V28";
    case 29: return "V29";
    case 30: return "V30";
    case 31: return "V31";
    }
    return "Invalid vector register";
  }
	};

// Architectural names, x0 through x31.
inline constexpr std::array<std::string_view, 32> XNAMES{
    "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",  "x8",  "x9",  "x10",
    "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19", "x20", "x21",
    "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"};

// ABI names. x8 is cannonically "s0", but "fp" will also be accepted for parsing.
inline constexpr std::array<std::string_view, 32> ABINAMES{
    "zero", "ra", "sp", "gp", "tp",  "t0",  "t1",  "t2",  "s0", "s1", "a0",
    "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7",  "s2",  "s3", "s4", "s5",
    "s6",   "s7", "s8", "s9", "s10", "s11", "t3",  "t4",  "t5", "t6"};

// Alternate spelling of x8 accepted during parsing.
inline constexpr std::string_view FRAME_POINTER_ALIAS = "fp";

constexpr XReg xreg(uint8_t reg) noexcept { return static_cast<XReg>(reg & 0x1F); }
constexpr ABIReg abireg(uint8_t reg) noexcept { return static_cast<ABIReg>(reg & 0x1F); }
// RISC-V register fields are five bits wide.
constexpr std::string_view xname(XReg reg) noexcept { return XNAMES[static_cast<uint8_t>(reg)]; }
constexpr std::string_view abiname(ABIReg reg) noexcept { return ABINAMES[static_cast<uint8_t>(reg)]; }
constexpr std::string_view xname(uint8_t reg) noexcept { return XNAMES[reg & 0x1F]; }
constexpr std::string_view abiname(uint8_t reg) noexcept { return ABINAMES[reg & 0x1F]; }

// Accepts either naming scheme, plus the alias for framepointer. nullopt if the name is not a register.
constexpr std::optional<uint8_t> parse_register(std::string_view name) noexcept {
  for (uint8_t i = 0; i < 32; ++i)
    if (name == XNAMES[i] || name == ABINAMES[i]) return i;
  if (name == FRAME_POINTER_ALIAS) return 8;
  return std::nullopt;
}

constexpr std::string_view FENCE_LETTERS = "iorw";
// Parse the letters of ordering string, setting only the bits that are present. If additional letters beyond iorw are
// present, return nullopt. Order-agnostic, and duplicates of the same letter are igored for compatiblility to GNU.
// Returns 0 if the string is empty or "0".
constexpr std::optional<uint8_t> parse_fence_ordering(std::string_view name) noexcept {
  if (name.empty()) return std::nullopt;
  else if (name == "0") return 0;
  uint8_t bits = 0;
  for (const char c : name) {
    const auto at = FENCE_LETTERS.find(c);
    if (at == std::string_view::npos) return std::nullopt;
    bits |= uint8_t(0b1000) >> at;
  }
  return bits;
}

// Rebuild ordering string in canonical order, or return 0 if empty.
inline std::string fence_ordering_name(uint8_t bits) {
  if ((bits & 0b1111) == 0) return "0";
  std::string out;
  for (std::size_t i = 0; i < FENCE_LETTERS.size(); ++i)
    if (bits & (0b1000 >> i)) out.push_back(FENCE_LETTERS[i]);
  return out;
}
} // namespace riscv
