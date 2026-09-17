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
#include <functional>
#include <memory>
#include <vector>
#include "core/ds/alloc/pagechain.hpp"
#include "core/formats/elf/packed_input_group.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/debugger/tvm_loader_backend.hpp"

namespace tvm {
class Interpreter;
}
class System;

// Convert all of PackedElfPhdr<B,E> to a single type so that TVM does not need to be aware of our template
// parameterization.
template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E>
tvm::SegmentDescriptor describe(const pepp::bts::PackedElfPhdr<B, E> &phdr, bits::span<const u8> data);

// Device::ID must refer to a Loadable*. Return true if the given segment should be loaded into that device, and false
// otherwise. An empty predicate should be equivalent to always returning true.
using SegmentPredicate = std::function<bool(Device::ID, const tvm::SegmentDescriptor &)>;

// Using the trace virtual machine infrastructure, initialize cores to their expected state after power-on, copy object
// code into each cores' memory, and update memory access permissions where necessary.
// If the input object code is unchanged, then you can re-use the same loader program across multiple runs of the same
// system, albiet you ought to reset all system devices between runs.
class Loader {
public:
  explicit Loader(System *sys);
  ~Loader();

  // Set a register to a constant value when the loader is run. The actual width of the value is determined by the
  // register's width. Returns false if the register is not found.
  bool set_register(RegisterScan::RegisterRef reg, u64 value);
  // Set a register to a value of a memory location when the loader is run. Used to intialize Pep/10's SP/PC to the
  // memory vector values, for example. Byteswap reverses byte word on its way in, in case yuor register and memory
  // endianness differ. Returns false if the register is not found.
  bool copy_word(RegisterScan::RegisterRef reg, Device::ID src, Address address, bool byteswap = false);

  // Take ownership of a group of ELF files and create a tvm program which copies the PT_LOAD segments into the given
  // device. The predicate filters PT_LOAD segments which should not be loaded into that device. If the predicate is
  // empty, all PT_LOAD segments will be loaded. Throws if the device does not exist or is not Loadable, or if the
  // group's format is incompatible with the loadable target.
  void add_group(pepp::bts::AnyElfGroup group, Device::ID device, SegmentPredicate predicate = {});

  // Reset interpreter's IP to the start of the loader program, and run until the program halts successfully or an error
  // occurs. Returns true if no errors were encountered and false if they were. If false, stop_cause will yield the
  // error.
  bool run();
  tvm::StopCause stop_cause() const;

  // Returns nullptr on invalid or out-of-bound handles, otherwise the segment descriptor for a handle.
  const tvm::SegmentDescriptor *segment(SegmentHandle handle) const;

private:
  // Extend the loader's program with more bytes. Throws if the program would exceed the buffer's capacity.
  void append(bits::span<const u8> bytes);
  // Adopt one validated segment and emit the LDSEGM which loads it into the device.
  void accept_segment(const tvm::SegmentDescriptor &desc, Device::ID dev);

  System *_sys = nullptr;
  // ELF files which have been transfered to this loader via add_group.
  std::vector<pepp::bts::AnyElfGroup> _groups;
  // A SegmentHandle's value is an index into this vector.
  std::vector<tvm::SegmentDescriptor> _segments;
  pepp::bts::Buffer *_program = nullptr;
  pepp::bts::Buffer::Location _start{};
  std::unique_ptr<tvm::Interpreter> _interpreter;
  // The interpreter owns the backend, but we need a view of it to supply loader-specific information.
  tvm::LoaderBackend *_backend = nullptr;
  bool _halted = false;
};

template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E>
tvm::SegmentDescriptor describe(const pepp::bts::PackedElfPhdr<B, E> &phdr, bits::span<const u8> data) {
  using namespace bits;
  tvm::SegmentDescriptor ret{.type = phdr.p_type, .vaddr = phdr.p_vaddr, .memsz = phdr.p_memsz, .data = data};
  const u32 flags = phdr.p_flags;
  if (flags & to_underlying(pepp::bts::SegmentFlags::PF_R)) ret.access |= Access::Read;
  if (flags & to_underlying(pepp::bts::SegmentFlags::PF_W)) ret.access |= Access::Write;
  if (flags & to_underlying(pepp::bts::SegmentFlags::PF_X)) ret.access |= Access::Execute;
  return ret;
}
