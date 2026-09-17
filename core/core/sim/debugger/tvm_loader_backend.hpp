/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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
#include <unordered_map>
#include "core/sim/api/memory.hpp"
#include "core/sim/debugger/tvm_apply_backend.hpp"

namespace tvm {

// One segment of any input file, abstracted away from the ELF library's representation to avoid tvm needing to know the
// ElfBits/ElfEndian aspect.
struct SegmentData {
  // Load address of the segment. If span is large than data, must zero-fill the difference.
  AddressSpan span{};
  bits::span<const u8> data{};
  Access access = Access::None; // Segment's requested access flahs.
};

// The backend powering the loader, capable of programming registers and loading segment data into memory.
class LoaderBackend : public ApplyBackend {
public:
  LoaderBackend(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system = nullptr);

  // Programs reference a segment with a 32-bit key composed of a file index (from the Loader) and a segment index (from
  // the ELF file). SegmentData must either outlive this class or be dropped via clear_segments().
  void register_segment(u16 file, u16 segment, const SegmentData &data);
  const SegmentData *segment(u16 file, u16 segment) const;
  void clear_segments();

  // While StopCause indicates a failure to load, it doesn't indicate which file+segment the vm was processing when it
  // stopped.
  struct Context {
    bool valid = false;
    u16 file = 0, segment = 0;
  };
  const Context &context() const { return _context; }

protected:
  // Mark context as valid and update file/segment.
  void set_context(u16 file, u16 segment);

  static constexpr u32 key_of(u16 file, u16 segment) { return (static_cast<u32>(file) << 16) | segment; }
  std::unordered_map<u32, SegmentData> _segments;
  Context _context{};
};

} // namespace tvm
