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
#include "core/sim/api/memory.hpp"
#include "core/sim/debugger/tvm_apply_backend.hpp"

class Loader;

namespace tvm {

// A single segment of some input file which is not concerned about the original bitness/endianness of the ELF header.
// Data bytes are unowned, and must outlive the descriptor.
struct SegmentDescriptor {
  u32 type = 0;  // p_type.
  u64 vaddr = 0; // Load (virtual) address for the segment.
  u64 memsz = 0; // Bytes in [data.size,memsz] must be zero-initialized and are not present in the file.
  Access access = Access::None;
  bits::span<const u8> data{}; // Length is p_filesz.

  // Returns true if the segment PT_LOAD with non-0 memory size.
  bool loadable() const;
  // Returns the span [vaddr, vaddr+memsz] as long as neither overflows the maximum value of Address. Otherwise, returns
  // nullopt.
  std::optional<AddressSpan> span() const;
};

// The backend powering the loader, capable of programming registers and loading segment data into memory. Failed memory
// or register accesses will trigger a hard stop rather only setting F bit and depending on in-program error handling.
class LoaderBackend : public ApplyBackend {
public:
  LoaderBackend(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system = nullptr,
                const Loader *loader = nullptr);

  void on_deltareg(MachineState &state, const DecodedOp::DeltaReg &op) override;
  void on_movmem2reg(MachineState &state, const DecodedOp::MovMem2Reg &op) override;
  void on_loadsegment(MachineState &state, const DecodedOp::LoadSegment &op) override;

  // While StopCause indicates a failure to load, it doesn't indicate which segment the vm was processing when it
  // stopped. Default-constructed if the machine has not reached a LDSEGM.
  SegmentHandle context() const { return _context; }

protected:
  const Loader *_loader = nullptr;
  SegmentHandle _context{};
};

} // namespace tvm
