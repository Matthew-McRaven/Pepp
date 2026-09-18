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

#include <string>
#include "core/formats/elf/enums_eheader.hpp"
#include "core/integers.h"
#include "core/sim/api/device.hpp"

class Loader;
struct Target;

// A unique identifier for one of the segments amongst all of the files currently in the Loader.
using SegmentHandle = pepp::OpaqueHandle<struct SegmentTag, u32>;

struct Loadable {
  static constexpr Device::Type TypeMask = Device::Type::Loadable;
  virtual ~Loadable() = default;

  virtual pepp::bts::ElfMachineType core_type() const noexcept = 0;
  virtual pepp::bts::ElfBits core_bits() const noexcept = 0;
  virtual pepp::bts::ElfEndian core_endian() const noexcept = 0;
  virtual void register_core_init(Loader &) = 0;

  // The kinds of memory attached to a core that a Loader might be interested in targeting.
  enum class MemoryKind : u8 { INVALID = 0, Instruction = 1, Data = 2, MicrocodeROM = 3 };
  // Return the target backing the requested kind of memory, or nullptr if this core has no memory of that kind.
  // Allows the loader to correctly load code, data, and microcode without needing to understand overall system arch.
  virtual Target *port(MemoryKind kind) = 0;

  // Render a listing-style for the instruction at the current PC, displaying the address, object code bytes, mnemonic,
  // and operands.
  virtual std::string stringize_next_instruction() const = 0;
  const Target *port(MemoryKind kind) const { return const_cast<Loadable *>(this)->port(kind); }
};
