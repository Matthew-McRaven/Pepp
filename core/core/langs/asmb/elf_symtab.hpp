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
#include <map>
#include <optional>
#include <string>
#include <zpp_bits.h>
#include "codegen.hpp"
#include "core/compile/ir_linear/attr_section.hpp"
#include "core/compile/ir_linear/line_base.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_io.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/integers.h"
#include "core/math/bitmanip/leb128.hpp"
#include "flat/flat_map.hpp"
#include "fmt/format.h"

namespace pepp::tc {

// Create a lookup data structure that converts IR pointers back to their listing line number.
using IR2ListingLinePair = std::pair<const LinearIR *, u32>;
struct IR2ListingLineComparator {
  bool operator()(const IR2ListingLinePair &lhs, const IR2ListingLinePair &rhs) const { return lhs.first < rhs.first; }
  bool operator()(LinearIR *const lhs, LinearIR *const rhs) const { return lhs < rhs; }
  bool operator()(const LinearIR *const lhs, const LinearIR *const rhs) const { return lhs < rhs; }
};
using IR2ListingLineMap = fc::flat_map<std::vector<IR2ListingLinePair>, IR2ListingLineComparator>;

// File names for .debug_line's source and listing units, and the directory they are relative to.
// Replaces the default when a non-empty string.
struct DebugLineFiles {
  std::optional<std::string> source, listing, directory;
};

struct ElfResult {
  pepp::bts::AnyGrowableElf elf;
  // Paired with elf's program headers: the [i]th constraint places the [i]th segment.
  std::vector<pepp::bts::SegmentLayoutConstraint> segments;
};

// One section per entry of `prog`, written straight into a packed file, then the symbol table once every section a
// symbol can name exists. Shared by every architecture, since only the file header differs between them.
ElfResult sections_to_elf(pepp::bts::ElfBits bits, pepp::bts::ElfEndian endian, pepp::bts::ElfMachineType machine,
                          const std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                          const ProgramObjectCodeResult &object_code, const pepp::core::symbol::LeafTable &symbols);

// Lay the file out and return its bytes. Empty if there is no file.
std::vector<u8> elf_bytes(ElfResult &result);
// Write elf output to a stream without an intermediate vector in memory.
void write_elf(ElfResult &result, std::ostream &out);
// Lay the file out and hand back a read-only view of it, so a just-assembled program can be loaded without a trip
// through the file system. With a path, the image is also left on disk there.
pepp::bts::AnyInputElf to_input_elf(ElfResult &result, std::optional<std::string> path = std::nullopt);
} // namespace pepp::tc
