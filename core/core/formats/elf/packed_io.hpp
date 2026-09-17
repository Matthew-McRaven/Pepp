/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
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

#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_ops.hpp"

namespace pepp::bts {

// Serialize into an in-memory contiguous buffer
template <ElfBits B, ElfEndian E>
std::vector<u8> elf_bytes(PackedElf<B, E> &elf, const std::vector<SegmentLayoutConstraint> *constraints = nullptr) {
  auto layout = calculate_layout(elf, constraints);
  std::vector<u8> ret(size_for_layout(layout), 0);
  write(ret, layout);
  return ret;
}

// Serialize into a stream, without an intermediate buffer.
template <ElfBits B, ElfEndian E>
void write_elf(PackedElf<B, E> &elf, std::ostream &out,
               const std::vector<SegmentLayoutConstraint> *constraints = nullptr) {
  write(out, calculate_layout(elf, constraints));
}

// Serialize to a memory-mapped file, which can be passed to the Loader without a re-read from the file system.
// If path is nullopt, the data is private to this process and will be released when the returned object is destroyed.
// Otherwise, the data is written to the disk.
template <ElfBits B, ElfEndian E>
std::unique_ptr<PackedInputElfFile<B, E>>
to_input_elf(PackedElf<B, E> &elf, const std::vector<SegmentLayoutConstraint> *constraints = nullptr,
             std::optional<std::string> path = std::nullopt) {
  auto layout = calculate_layout(elf, constraints);
  if (path) {
    write_mmap(*path, layout);
    return std::make_unique<PackedInputElfFile<B, E>>(*path);
  }
  std::vector<char> bytes(size_for_layout(layout), 0);
  write(std::span<u8>(reinterpret_cast<u8 *>(bytes.data()), bytes.size()), layout);
  return std::make_unique<PackedInputElfFile<B, E>>(std::make_shared<BufferSource>(std::move(bytes)));
}

// Overloads which dispatch to the correct implementation at runtime.
std::vector<u8> elf_bytes(AnyGrowableElf &elf, const std::vector<SegmentLayoutConstraint> *constraints = nullptr);
void write_elf(AnyGrowableElf &elf, std::ostream &out,
               const std::vector<SegmentLayoutConstraint> *constraints = nullptr);
AnyInputElf to_input_elf(AnyGrowableElf &elf, const std::vector<SegmentLayoutConstraint> *constraints = nullptr,
                         std::optional<std::string> path = std::nullopt);
} // namespace pepp::bts
