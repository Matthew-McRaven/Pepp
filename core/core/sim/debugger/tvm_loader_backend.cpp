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
#include "core/sim/debugger/tvm_loader_backend.hpp"
#include <limits>
#include "core/formats/elf/enums_segments.hpp"
#include "core/sim/api/loadable.hpp"
#include "core/sim/loader.hpp"
#include "core/sim/system.hpp"

// Force the machine to hard-stop if it has not already.
static void stop_if_refused(tvm::MachineState &state) {
  if (!state.stopped() && state.csrs.F) state.hard_stop(tvm::StopCause::AccessRefused);
}

namespace tvm {

bool SegmentDescriptor::loadable() const {
  return type == bits::to_underlying(pepp::bts::SegmentType::PT_LOAD) && memsz > 0;
}

std::optional<AddressSpan> SegmentDescriptor::span() const {
  // Our loader works on 32-bit systems, so we must ensure that the VA span is in range.
  if (memsz == 0 || memsz - 1 > std::numeric_limits<Address>::max() - vaddr) return std::nullopt;
  return AddressSpan(static_cast<Address>(vaddr), static_cast<Address>(vaddr + memsz - 1));
}

LoaderBackend::LoaderBackend(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system, const Loader *loader)
    : ApplyBackend(std::move(mgr), system), _loader(loader) {}

void LoaderBackend::on_deltareg(MachineState &state, const DecodedOp::DeltaReg &op) {
  ApplyBackend::on_deltareg(state, op);
  // Loader does not include BRF / error handling, so treat a failed reg write as a hard stop.
  stop_if_refused(state);
}

void LoaderBackend::on_movmem2reg(MachineState &state, const DecodedOp::MovMem2Reg &op) {
  // Loader does not include BRF / error handling, so treat a failed memory-to-reg write as a hard stop.
  ApplyBackend::on_movmem2reg(state, op);
  stop_if_refused(state);
}

void LoaderBackend::on_loadsegment(MachineState &state, const DecodedOp::LoadSegment &op) {
  // Whatever the segment overwrites is gone, so a load cannot be stepped back over.
  if (!is_forward()) return state.hard_stop(StopCause::NotInvertible);
  else if (state.csrs.TR == 1) return state.hard_stop(StopCause::WrongTR);
  else if (_system == nullptr) return state.hard_stop(StopCause::MissingSystem);
  // StopCauses can't indicate the failing segment because the stop cause is too narrow.
  // A useful diagnostic message would include the segment which failed, so record it in _context.
  _context = op.src;

  auto *dev = _system->find_by_id(op.dst);
  if (dev == nullptr) return state.hard_stop(StopCause::TargetInvalid);
  auto *loadable = dev->capability<Loadable>();
  if (loadable == nullptr) return state.hard_stop(StopCause::TargetNotLoadable);

  // Ensure loading fials if op names an invalid memory kind or an invalid kind for that target.
  Target *target = nullptr;
  switch (op.kind) {
  case Loadable::MemoryKind::Instruction: [[fallthrough]];
  case Loadable::MemoryKind::Data: [[fallthrough]];
  case Loadable::MemoryKind::MicrocodeROM: target = loadable->port(op.kind); break;
  default: break;
  }
  if (target == nullptr) return state.hard_stop(StopCause::TargetNotMemory);

  const auto *segment = _loader == nullptr ? nullptr : _loader->segment(op.src);
  if (segment == nullptr) return state.hard_stop(StopCause::SegmentUnknown);
  const auto span = segment->span();
  if (!span) return state.hard_stop(StopCause::SegmentUnknown);

  if (!try_access([&] { target->load(*span, segment->data, segment->access); }))
    state.hard_stop(StopCause::AccessRefused);
}

} // namespace tvm
