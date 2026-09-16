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
#include "core/math/bitmanip/span.hpp"
#include "core/sim/api/device.hpp"

struct Loadable {
  static constexpr Device::Type TypeMask = Device::Type::Loadable;
  virtual ~Loadable() = default;

  virtual pepp::bts::ElfMachineType core_type() const noexcept = 0;
  virtual pepp::bts::ElfBits core_bits() const noexcept = 0;
  virtual pepp::bts::ElfEndian core_endian() const noexcept = 0;
};
