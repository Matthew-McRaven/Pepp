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
#include <optional>
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

// A file opened by the loader.
struct ElfImage {
  virtual ~ElfImage() = default;
  // The segment at this index, or nothing when the index names no loadable segment of this file.
  virtual std::optional<SegmentData> segment(u16 index) const = 0;
};

// The backend powering the loader, capable of programming registers and loading segment data into memory.
class LoaderBackend : public ApplyBackend {
public:
  LoaderBackend(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system = nullptr);

  // Programs name files by index, which the loader assigns when it opens them. Registering the same index twice
  // replaces the previous file. The image must outlive every run of a program which names it.
  void set_file(u16 index, const ElfImage *image);
  const ElfImage *file(u16 index) const;
  void clear_files();

  // Which file and segment the machine was working on when it stopped. StopCause says what went wrong, and this says
  // where, so that the loader can report something more useful than "hard stop".
  struct Context {
    bool valid = false;
    u16 file = 0, segment = 0;
  };
  const Context &context() const { return _context; }

protected:
  // Note the segment a handler is about to work on, so that a stop can be attributed to it.
  void set_context(u16 file, u16 segment);

  std::unordered_map<u16, const ElfImage *> _files;
  Context _context{};
};

} // namespace tvm
