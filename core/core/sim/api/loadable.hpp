/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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
 */
#pragma once

#include "core/formats/elf/enums_eheader.hpp"
#include "core/integers.h"
#include "core/sim/api/device.hpp"

struct Target;
struct Loadable {
  static constexpr Device::Type TypeMask = Device::Type::Loadable;
  virtual ~Loadable() = default;

  virtual pepp::bts::ElfMachineType core_type() const noexcept = 0;
  virtual pepp::bts::ElfBits core_bits() const noexcept = 0;
  virtual pepp::bts::ElfEndian core_endian() const noexcept = 0;

  // The kinds of memory attached to a core that a Loader might be interested in targeting.
  enum class MemoryKind : u8 { Instruction, Data, MicrocodeROM };
  // Return the target backing the requested kind of memory, or nullptr if this core has no memory of that kind.
  // Allows the loader to correctly load code, data, and microcode without needing to understand overall system arch.
  virtual Target *port(MemoryKind kind) = 0;
  const Target *port(MemoryKind kind) const { return const_cast<Loadable *>(this)->port(kind); }
};
