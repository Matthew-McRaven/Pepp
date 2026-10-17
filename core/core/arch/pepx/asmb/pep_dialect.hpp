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
#include <optional>
#include <span>
#include <string_view>
#include "core/arch/pepx/asmb/pep_mnemonics.hpp"
#include "core/arch/pepx/isa/pep_opcodes.hpp"
#include "core/arch/pepx/isa/pep_registers.hpp"

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

// The details about a Pep assembly language needed by the assembler.
struct Dialect {
  MnemonicMap mnemonics;
  std::span<const NamedEnum<AddrMode>> addressing_modes = addressing_mode_names;
  std::span<const NamedEnum<Register>> registers = register_names;

  auto alternatives(std::string_view name) const { return mnemonics.equal_range(name); }
  // Match case-insensitively via linear scan. If this becomes a common operation, construct a pair of maps from the
  // span like we did in the old ISA classes.
  std::optional<AddrMode> parse_addressing_mode(std::string_view name) const;
  std::string_view name(AddrMode mode) const;
  std::optional<Register> parse_register(std::string_view name) const;
  std::string_view name(Register reg) const;
};
} // namespace pep
