#include "core/sim/debugger/tvm_apply_backend.hpp"
#include <cstring>
#include <stdexcept>
#include "core/sim/api/memory.hpp"
#include "core/sim/memory/errors.hpp"
#include "core/sim/system.hpp"

namespace {
const Operation rw_cmp(Operation::Type::BufferInternal, Operation::Kind::data);

// Run a target acccess and report if it succeded, used to catch bad access from targets and set the F bit accordingly.
template <typename Fn> bool try_access(Fn &&fn) {
  try {
    fn();
    return true;
  } catch (const Error &) {
    // A Target refused the access: out of range, unmapped, and so on.
    return false;
  } catch (const std::runtime_error &) {
    // RegisterScan reports the same class of refusal -- not readable, not writable, no such device -- by throwing,
    // and it throws plain std::runtime_error rather than Error. Without this catch a program touching a read-only
    // register would unwind out of the interpreter entirely instead of setting F, which is the opposite of how
    // every other refused access behaves. RegisterScan ought to grow a typed exception; until it does, this is
    // where the two hierarchies are reconciled.
    return false;
  }
}
} // namespace

namespace tvm {

ApplyBackend::ApplyBackend(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system)
    : _mgr(std::move(mgr)), _system(system) {
  if (_system) _scan = _system->register_scan();
  else _scan = nullptr;
}

void ApplyBackend::on_setmem(MachineState &state, const tvm::DecodedOp::SetMem &op) {
  using StopCause = tvm::StopCause;
  // Not in register mode or there is no system. Either way, comparsion will fail.
  if (state.csrs.TR == 1) return state.hard_stop(StopCause::WrongTR);
  else if (_system == nullptr) return state.hard_stop(StopCause::MissingSystem);

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

  // The read-xor-write is one logical access: if the read fails there is nothing meaningful to write back.
  const bool ok = try_access([&] {
    bits::span<const u8> payload = data;
    // If xor-encoded, perform extract data into temporary buffer and ^ our data into that temp
    if (op.xor_encoded) {
      if (_tmp.size() < op.size) _tmp.resize(op.size);
      bits::span<u8> tmp(_tmp.data(), op.size);
      // Lie about access type for this access to avoid side effects.
      target->read(op.offset, tmp, rw_cmp);
      bits::inplace_xor(tmp, data);
      // "swap" temp buffer into data
      payload = tmp;
    }
    target->write(op.offset, payload, op.access);
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

void ApplyBackend::on_setreg(MachineState &state, const tvm::DecodedOp::SetReg &op) {
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

  // If size mismatch, then we would have to do a partial write, and we'd need to compute host/guest endianness
  // mismatch. That sounds annoying, so skip.
  if (reg->byte_width != op.size) return state.hard_stop(StopCause::RegisterSizeMismatch);
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
  bits::span<const u8> data = dbuff->span().subspan(op.data.lo, op.size);

  const bool ok = try_access([&] {
    // If data is XOR-encoded, we first need to extract the current register value.
    // TODO: this should be a BufferInternal read, not a normal one!
    if (op.xor_encoded) {
      switch (reg->byte_width) {
      case 1: expected ^= _scan->read<u8>(op.reg); break;
      case 2: expected ^= _scan->read<u16>(op.reg); break;
      case 4: expected ^= _scan->read<u32>(op.reg); break;
      default: break;
      }
    }
    // RegisterScanner handles field vs register writes.
    switch (reg->byte_width) {
    case 1: _scan->write<u8>(op.reg, (u8)expected); break;
    case 2: _scan->write<u16>(op.reg, (u16)expected); break;
    case 4: _scan->write<u32>(op.reg, (u32)expected); break;
    default: break;
    }
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
    case 1: actual = _scan->read<u8>(op.reg); break;
    case 2: actual = _scan->read<u16>(op.reg); break;
    case 4: actual = _scan->read<u32>(op.reg); break;
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
  // Refuse through the ISA's own failure channel rather than by throwing. TRADDR decodes and encodes like any other
  // opcode, so a trace containing one is not a programming error in the driver -- it is a program this backend
  // cannot run, and it should stop the machine the way every other refusal does instead of unwinding out of
  // Interpreter::step and past run_each.
  state.hard_stop(tvm::StopCause::Unimplemented);
}

} // namespace tvm
