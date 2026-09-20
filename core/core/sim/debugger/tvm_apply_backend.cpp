#include "core/sim/debugger/tvm_apply_backend.hpp"
#include <cstring>
#include <stdexcept>
#include "core/math/bitmanip/copy.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"
#include "core/sim/memory/errors.hpp"
#include "core/sim/memory/io/fifo.hpp"
#include "core/sim/system.hpp"

namespace {
const Operation rw_cmp(Operation::Type::BufferInternal, Operation::Kind::data);
} // namespace

namespace tvm {

ApplyBackend::ApplyBackend(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system)
    : _mgr(std::move(mgr)), _system(system) {
  if (_system) _scan = _system->register_scan();
  else _scan = nullptr;
}

void ApplyBackend::on_deltamem(MachineState &state, const tvm::DecodedOp::DeltaMem &op) {
  using StopCause = tvm::StopCause;
  // Not in register mode or there is no system. Either way, comparsion will fail.
  if (state.csrs.TR == 1) return state.hard_stop(StopCause::WrongTR);
  else if (_system == nullptr) return state.hard_stop(StopCause::MissingSystem);
  // Add does its arithmetic in a u64, so neither the operand nor the delta may be wider than one. This is a property
  // of the instruction rather than of the machine, so it is checked before anything is resolved.
  else if (op.kind == tvm::Delta::Add && op.size > sizeof(u64)) return state.hard_stop(StopCause::StepWidthIllegal);

  // Attempt to convert our ID to a target;
  auto dev = _system->find_by_id(op.target);
  if (!dev) return state.hard_stop(StopCause::TargetInvalid);
  auto target = dev->capability<Target>();
  if (!target) return state.hard_stop(StopCause::TargetNotMemory);

  auto dbuff = _mgr->find((pepp::bts::Buffer::ID)op.data.hi);
  // Prevent access to invalid dbuff / past its end
  if (!dbuff) return state.hard_stop(StopCause::InvalidDBuffer);
  else if ((size_t)op.data.lo + op.size > dbuff->span().size()) return state.hard_stop(StopCause::InvalidDBuffer);

  bits::span<const u8> data = dbuff->span().subspan(op.data.lo, op.size);

  // Perform read-modify-write in a try block so we can catch exceptions and set F accordingly.
  const bool ok = try_access([&] {
    bits::span<const u8> payload = data;
    switch (op.kind) {
    // The payload is the destination's new contents already.
    case tvm::Delta::Assign: break;
    // Extract the current contents into a temporary buffer and ^ our data into that temp.
    case tvm::Delta::Xor: {
      if (_tmp.size() < op.size) _tmp.resize(op.size);
      bits::span<u8> tmp(_tmp.data(), op.size);
      // Lie about access type for this access to avoid side effects.
      target->read(op.offset, tmp, rw_cmp);
      bits::inplace_xor(tmp, data);
      // "swap" temp buffer into data
      payload = tmp;
      break;
    }
    case tvm::Delta::Add: {
      if (_tmp.size() < op.size) _tmp.resize(op.size);
      bits::span<u8> tmp(_tmp.data(), op.size);
      target->read(op.offset, tmp, rw_cmp);
      // memcpy_endian keeps the low-order bytes in either direction, so a sum that overflows the operand wraps
      // within it rather than spilling into a neighbour.
      const u64 sum = bits::memcpy_endian<u64>(tmp, op.order) + directed_delta(signed_le(data));
      bits::memcpy_endian(tmp, op.order, sum);
      payload = tmp;
      break;
    }
    }
    // When undoing, we must switch our access type, otherwise we create additional spurious traces.
    target->write(op.offset, payload, effective_access(op.access));
  });
  state.csrs.F = ok ? 0 : 1;
}

void ApplyBackend::on_cmpmem(MachineState &state, const tvm::DecodedOp::CmpMem &op) {
  using StopCause = tvm::StopCause;
  // Not in register mode or there is no system. Either way, comparsion will fail.
  if (state.csrs.TR == 1) return state.hard_stop(StopCause::WrongTR);
  else if (_system == nullptr) return state.hard_stop(StopCause::MissingSystem);

  // Attempt to convert our ID to a target;
  auto dev = _system->find_by_id(op.target);
  if (!dev) return state.hard_stop(StopCause::TargetInvalid);
  auto target = dev->capability<Target>();
  if (!target) return state.hard_stop(StopCause::TargetNotMemory);

  if (_tmp.size() < op.size) _tmp.resize(op.size);
  bits::span<u8> actual(_tmp.data(), op.size);
  auto dbuff = _mgr->find((pepp::bts::Buffer::ID)op.data.hi);
  // Prevent access to invalid dbuff / past its end
  if (!dbuff) return state.hard_stop(StopCause::InvalidDBuffer);
  else if ((size_t)op.data.lo + op.size > dbuff->span().size()) return state.hard_stop(StopCause::InvalidDBuffer);
  auto expected = dbuff->span().subspan(op.data.lo, op.size);
  if (!try_access([&] { target->read(op.offset, actual, rw_cmp); })) {
    // Z/N are left alone: a read that never happened has nothing to say about ordering, and overwriting them would
    // make a failed compare indistinguishable from a successful "not equal".
    state.csrs.F = 1;
    return;
  }
  state.csrs.F = 0;
  auto cmp = std::memcmp(actual.data(), expected.data(), op.size);
  // Set conditions according to memcmp result.
  if (cmp == 0) state.csrs.Z = 1, state.csrs.N = 0;
  else if (cmp < 0) state.csrs.Z = 0, state.csrs.N = 1;
  else state.csrs.Z = 0, state.csrs.N = 0;
}

void ApplyBackend::on_clrmem(MachineState &state, const tvm::DecodedOp::ClrMem &op) {
  using StopCause = tvm::StopCause;
  // Not in register mode or there is no system. Either way, comparsion will fail.
  if (state.csrs.TR == 1) return state.hard_stop(StopCause::WrongTR);
  else if (_system == nullptr) return state.hard_stop(StopCause::MissingSystem);

  // Attempt to convert our ID to a target;
  auto dev = _system->find_by_id(op.target);
  if (!dev) return state.hard_stop(StopCause::TargetInvalid);
  auto target = dev->capability<Target>();
  if (!target) return state.hard_stop(StopCause::TargetNotMemory);

  state.csrs.F = try_access([&] { target->clear(op.data); }) ? 0 : 1;
}

void ApplyBackend::on_deltareg(MachineState &state, const tvm::DecodedOp::DeltaReg &op) {
  using StopCause = tvm::StopCause;
  // Not in register mode or there is no system. Either way, comparsion will fail.
  if (state.csrs.TR == 0) return state.hard_stop(StopCause::WrongTR);
  else if (_scan == nullptr) return state.hard_stop(StopCause::MissingSystem);

  // Attempt to convert our ID registers
  auto pair = _scan->resolve(op.reg);
  if (pair.first == nullptr) return state.hard_stop(StopCause::RegisterInvalid);
  // Manually unpack to make debugging easier.
  auto reg = pair.first;

  // Prevent access to invalid dbuff / past its end
  auto dbuff = _mgr->find((pepp::bts::Buffer::ID)op.data.hi);
  if (!dbuff) return state.hard_stop(StopCause::InvalidDBuffer);
  else if ((size_t)op.data.lo + op.size > dbuff->span().size()) return state.hard_stop(StopCause::InvalidDBuffer);

  bits::span<const u8> data = dbuff->span().subspan(op.data.lo, op.size);

  // Everything below goes through 64 accessors, which apply the register's own byte order and mask to a field.
  if (reg->byte_width == 0 || reg->byte_width > sizeof(u64)) return state.hard_stop(StopCause::RegisterWidthIllegal);
  // STEPREG can have a smaller size than the register. Other operations must match size exactly.
  else if (op.kind != tvm::Delta::Add && reg->byte_width != op.size)
    return state.hard_stop(StopCause::RegisterSizeMismatch);
  // Addition uses C++ primitives, so we are limited to the maximum size of an integer.
  else if (op.kind == tvm::Delta::Add && op.size > sizeof(u64)) return state.hard_stop(StopCause::StepWidthIllegal);

  // Immediates are little-endian throughout this ISA.
  const u64 payload = bits::memcpy_endian<u64>(data, bits::Order::LittleEndian);

  const bool ok = try_access([&] {
    u64 value = 0;
    switch (op.kind) {
    // The payload is the register's new contents already.
    case tvm::Delta::Assign: value = payload; break;
    case tvm::Delta::Xor: value = payload ^ _scan->read<u64>(op.reg, RegisterScan::Level::Host); break;
    case tvm::Delta::Add:
      value = _scan->read<u64>(op.reg, RegisterScan::Level::Host) + directed_delta(signed_le(data));
      break;
    }
    // RegisterScanner handles field vs register writes. Level::Host because this is the machinery restoring state,
    // not the user editing it: a counter may be read-only to the guest and still land here.
    _scan->write<u64>(op.reg, value, RegisterScan::Level::Host);
  });
  state.csrs.F = !ok;
}

void ApplyBackend::on_cmpreg(MachineState &state, const tvm::DecodedOp::CmpReg &op) {
  using StopCause = tvm::StopCause;
  // Not in register mode or there is no system. Either way, comparsion will fail.
  if (state.csrs.TR == 0) return state.hard_stop(StopCause::WrongTR);
  else if (_scan == nullptr) return state.hard_stop(StopCause::MissingSystem);

  // Attempt to convert our ID registers
  auto pair = _scan->resolve(op.reg);
  if (pair.first == nullptr) return state.hard_stop(StopCause::RegisterInvalid);
  // Manually unpack to make debugging easier.
  auto reg = pair.first;
  auto field = pair.second;

  // Prevent access to invalid dbuff / past its end
  auto dbuff = _mgr->find((pepp::bts::Buffer::ID)op.data.hi);
  if (!dbuff) return state.hard_stop(StopCause::InvalidDBuffer);
  else if ((size_t)op.data.lo + op.size > dbuff->span().size()) return state.hard_stop(StopCause::InvalidDBuffer);
  // If size mismatch, then we would have to do a partial comparison.
  // That sounds annoying, so skip.
  else if (reg->byte_width != op.size) return state.hard_stop(StopCause::RegisterSizeMismatch);

  const auto dat = (pepp::bts::Buffer::ID)op.data.hi;
  u64 expected = 0;
  switch (reg->byte_width) {
  case 1: expected = read16(*_mgr, state, dat, op.data.lo) & 0xff; break;
  case 2: expected = read16(*_mgr, state, dat, op.data.lo); break;
  case 4:
    expected = ((u32)read16(*_mgr, state, dat, op.data.lo + 2) << 16) | read16(*_mgr, state, dat, op.data.lo);
    break;
  default: return state.hard_stop(StopCause::RegisterWidthIllegal);
  }

  u64 actual = 0;
  const bool ok = try_access([&] {
    switch (reg->byte_width) {
    case 1: actual = _scan->read<u8>(op.reg, RegisterScan::Level::Host); break;
    case 2: actual = _scan->read<u16>(op.reg, RegisterScan::Level::Host); break;
    case 4: actual = _scan->read<u32>(op.reg, RegisterScan::Level::Host); break;
    default: break;
    }
  });
  if (!ok) {
    state.csrs.F = 1;
    return;
  }

  // If we are accessing a field, we should mask our expect value down to the size of the field, which is what
  // _scan->read does.
  if (field != nullptr) {
    const u64 mask = field->bit_width >= 64 ? ~0ULL : (1ULL << field->bit_width) - 1;
    expected &= mask;
  }

  state.csrs.F = 0;
  if (actual == expected) state.csrs.Z = 1, state.csrs.N = 0;
  else if (actual < expected) state.csrs.Z = 0, state.csrs.N = 1;
  else state.csrs.Z = 0, state.csrs.N = 0;
}

void ApplyBackend::on_clrreg(MachineState &state, const tvm::DecodedOp::ClrReg &op) {
  using StopCause = tvm::StopCause;
  // Not in register mode or there is no scan. Either way, the clear will fail.
  if (state.csrs.TR == 0) return state.hard_stop(StopCause::WrongTR);
  else if (_scan == nullptr) return state.hard_stop(StopCause::MissingSystem);
  // Resolve up front so a bad ID stops the machine the way CMPREG does. clear() reports it by throwing, and that is
  // a std::runtime_error rather than an Error, so it would sail past try_access and out of the blaster entirely.
  else if (_scan->resolve(op.reg).first == nullptr) return state.hard_stop(StopCause::RegisterInvalid);

  state.csrs.F = try_access([&] { _scan->clear(op.reg); }) ? 0 : 1;
}

void ApplyBackend::on_traddr(MachineState &state, const tvm::DecodedOp::TRADDR &op) {
  state.hard_stop(tvm::StopCause::Unimplemented);
}

void ApplyBackend::on_mmio(MachineState &state, const DecodedOp::MMIO &op) {
  // Not in target mode or there is no system.
  if (state.csrs.TR == 1) return state.hard_stop(StopCause::WrongTR);
  else if (_system == nullptr) return state.hard_stop(tvm::StopCause::MissingSystem);

  // Attempt to convert our ID to a FIFORegister
  auto dev = _system->find_by_id(op.target);
  if (!dev) return state.hard_stop(StopCause::TargetInvalid);
  auto target = dev->capability<Target>();
  if (!target) return state.hard_stop(StopCause::TargetNotMemory);
  auto fifo = dynamic_cast<FIFORegister *>(target);
  if (!fifo) return state.hard_stop(StopCause::TargetNotFIFO);

  // Traces cannot be replayed from the middle; you have to start from one end or the other.
  // So, we can operate on the head/tail of the FIFO directly rather than having to modify indices.
  if (is_forward()) {
    if (op.write) fifo->output().push(op.data);
    // Not a bare push: the byte may already be queued, either because an undo stepped back over it or because the
    // user typed ahead. advance_input queues it only when the read position has run off the end.
    else fifo->advance_input(op.data);
  } else {
    if (op.write) fifo->output().pop_back();
    else fifo->rewind_input();
  }
}

void ApplyBackend::on_movmem2reg(MachineState &state, const DecodedOp::MovMem2Reg &op) {
  using StopCause = tvm::StopCause;
  // Register's old value is not preserved
  if (!is_forward()) return state.hard_stop(StopCause::NotInvertible);
  else if (state.csrs.TR == 0) return state.hard_stop(StopCause::WrongTR);
  else if (_system == nullptr || _scan == nullptr) return state.hard_stop(StopCause::MissingSystem);

  // Attempt to convert our source to a Target
  auto dev = _system->find_by_id(op.src);
  if (!dev) return state.hard_stop(StopCause::TargetInvalid);
  auto target = dev->capability<Target>();
  if (!target) return state.hard_stop(StopCause::TargetNotMemory);
  // Attempt to convert our ID registers
  auto pair = _scan->resolve(op.dst);
  if (pair.first == nullptr) return state.hard_stop(StopCause::RegisterInvalid);
  // Manually unpack to make debugging easier.
  auto reg = pair.first;

  // Size is inferred from the register's width.
  if (reg->byte_width == 0 || reg->byte_width > sizeof(u64)) return state.hard_stop(StopCause::RegisterWidthIllegal);

  if (_tmp.size() < reg->byte_width) _tmp.resize(reg->byte_width);
  bits::span<u8> raw(_tmp.data(), reg->byte_width);
  const bool ok = try_access([&] {
    const auto byteswap = op.byteswap ? RegisterScan::Byteswap::Always : RegisterScan::Byteswap::Never;
    target->read(op.offset, raw, effective_access(op.access));
    // Always perform write at host level, since this is intended to be used during the loading process.
    _scan->write(op.dst, raw, byteswap, RegisterScan::Level::Host);
  });
  state.csrs.F = !ok;
}

void ApplyBackend::on_loadsegment(MachineState &state, const DecodedOp::LoadSegment &op) {
  state.hard_stop(StopCause::Unimplemented);
}

TraceApplyBackend::TraceApplyBackend(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system,
                                     tvm::TraceBuffer *tb)
    : ApplyBackend(std::move(mgr), system), _tb(tb) {}

void TraceApplyBackend::on_stcall(MachineState &state, const tvm::DecodedOp::STCALL &op) {
  if (_tb == nullptr) return ApplyBackend::on_stcall(state, op);
  const auto target = _tb->stencil_location(op.index);
  if (target.id == pepp::bts::Buffer::ID{0}) return state.hard_stop(tvm::StopCause::StencilUnknown);

  if (op.returns_to_halt) {
    const auto halt = _tb->halt_location();
    state.push(tvm::SegmentPair{.hi = halt.id.value, .lo = halt.offset});
  } else state.push(state.regs.IP);

  state.regs.IP = tvm::SegmentPair{.hi = target.id.value, .lo = target.offset};
}

void TraceApplyBackend::on_callhalt(MachineState &state, const tvm::DecodedOp::CallHalt &op) {
  if (_tb == nullptr) return ApplyBackend::on_callhalt(state, op);
  const auto halt = _tb->halt_location();
  state.push(tvm::SegmentPair{.hi = halt.id.value, .lo = halt.offset});
  state.regs.IP = op.next_ip;
}

void TraceApplyBackend::on_dpincr(MachineState &state, const tvm::DecodedOp::DPIncr &op) {
  // Without a buffer there is no chain to follow, so defer to the base class's behavior.
  if (_tb == nullptr) return ApplyBackend::on_dpincr(state, op);

  auto &regs = state.regs;
  regs.DS = op.DS;

  // Use signed 32-bit arithmetic so we can detect both overflow and underflow cleanly.
  int32_t new_lo = static_cast<int32_t>(regs.DP.lo) + static_cast<int16_t>(op.dp_incr);
  constexpr int32_t BUF_SIZE = static_cast<int32_t>(pepp::bts::Buffer::SIZE);

  if (new_lo >= BUF_SIZE) {
    // Forward overflow: go to successor buffer.
    auto succ = _tb->data_successor(pepp::bts::Buffer::ID{regs.DP.hi});
    if (succ == pepp::bts::Buffer::ID{0}) return state.hard_stop(tvm::StopCause::InvalidDBuffer);
    regs.DP.hi = succ.value;
    regs.DP.lo = static_cast<u16>(new_lo - BUF_SIZE);
  } else if (new_lo < 0) {
    // Backward underflow: go to predecessor buffer.
    auto pred = _tb->data_predecessor(pepp::bts::Buffer::ID{regs.DP.hi});
    if (pred == pepp::bts::Buffer::ID{0}) return state.hard_stop(tvm::StopCause::InvalidDBuffer);
    regs.DP.hi = pred.value;
    regs.DP.lo = static_cast<u16>(new_lo + BUF_SIZE);
  } else {
    regs.DP.lo = static_cast<u16>(new_lo);
  }
}

} // namespace tvm
