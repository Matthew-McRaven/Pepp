/*
 * /Copyright (c) 2026. Stanley Warford, Matthew McRaven
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
#pragma once
#include <array>
#include <initializer_list>
#include <optional>
#include <span>
#include <string_view>
#include "core/arch/pepx/asmb/pep_mnemonics.hpp"
#include "core/arch/pepx/isa/pep_opcodes.hpp"
#include "core/arch/pepx/isa/pep_registers.hpp"
#include "core/integers.h"

namespace pep {
template <typename E> struct NamedEnum {
  E value;
  std::string_view name;
};

// Names shared by every Pep architecture.
inline constexpr auto addressing_mode_names = std::to_array<NamedEnum<AddrMode>>({
    {AddrMode::I, "i"},
    {AddrMode::D, "d"},
    {AddrMode::N, "n"},
    {AddrMode::S, "s"},
    {AddrMode::SF, "sf"},
    {AddrMode::X, "x"},
    {AddrMode::SX, "sx"},
    {AddrMode::SFX, "sfx"},
});
inline constexpr auto register_names = std::to_array<NamedEnum<Register>>({
    {Register::A, "A"},
    {Register::X, "X"},
    {Register::SP, "SP"},
    {Register::PC, "PC"},
    {Register::IS, "IS"},
    {Register::OS, "OS"},
});

// Dot commands accepted by the assembler
enum class Directive : u8 {
  ADDRSS,
  ALIGN,
  ASCII,
  BLOCK,
  BURN,
  BYTE,
  END,
  EQUATE,
  EXPORT,
  INPUT,
  ORG,
  OUTPUT,
  SCALL,
  SECTION,
  WORD,
  IF,
  ELSEIF,
  ELSE,
  ENDIF,
  MACRO,
  ENDM,
};
using DirectiveMask = u32;
constexpr DirectiveMask mask_of(Directive d) { return static_cast<DirectiveMask>(1u << static_cast<u8>(d)); }
constexpr DirectiveMask mask_of(std::initializer_list<Directive> ds) {
  DirectiveMask ret = 0;
  for (auto d : ds) ret |= mask_of(d);
  return ret;
}

inline constexpr auto directive_names = std::to_array<NamedEnum<Directive>>({
    {Directive::ADDRSS, "ADDRSS"}, {Directive::ALIGN, "ALIGN"},   {Directive::ASCII, "ASCII"},
    {Directive::BLOCK, "BLOCK"},   {Directive::BURN, "BURN"},     {Directive::BYTE, "BYTE"},
    {Directive::END, "END"},       {Directive::EQUATE, "EQUATE"}, {Directive::EXPORT, "EXPORT"},
    {Directive::INPUT, "INPUT"},   {Directive::ORG, "ORG"},       {Directive::OUTPUT, "OUTPUT"},
    {Directive::SCALL, "SCALL"},   {Directive::SECTION, "SECTION"}, {Directive::WORD, "WORD"},
    {Directive::IF, "IF"},         {Directive::ELSEIF, "ELSEIF"}, {Directive::ELSE, "ELSE"},
    {Directive::ENDIF, "ENDIF"},   {Directive::MACRO, "MACRO"},   {Directive::ENDM, "ENDM"},
});

// The details about a Pep assembly language needed by the assembler.
struct Dialect {
  MnemonicMap mnemonics;
  DirectiveMask directives = 0;
  std::span<const NamedEnum<AddrMode>> addressing_modes = addressing_mode_names;
  std::span<const NamedEnum<Register>> registers = register_names;

  auto alternatives(std::string_view name) const { return mnemonics.equal_range(name); }
  // Match case-insensitively via linear scan. If this becomes a common operation, construct a pair of maps from the
  // span like we did in the old ISA classes.
  std::optional<AddrMode> parse_addressing_mode(std::string_view name) const;
  std::string_view name(AddrMode mode) const;
  std::optional<Register> parse_register(std::string_view name) const;
  std::string_view name(Register reg) const;
  // Nullopt if the name is unknown or this dialect does not accept it.
  std::optional<Directive> parse_directive(std::string_view name) const;
  std::string_view name(Directive directive) const;
};
} // namespace pep
