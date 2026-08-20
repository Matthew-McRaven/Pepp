#include "core/sim/debugger/trace_recorder.hpp"
#include <algorithm>
#include <array>
#include <utility>
#include "core/math/bitmanip/copy.hpp"
#include "core/sim/debugger/tvm_encoding.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"

namespace trace {

bool Recorder::traced() const { return _tb != nullptr && _tb->traced(_emitter); }

// --- Initiator side ---

Recorder::Instruction::Instruction(const Recorder &rec) {
  if (!rec.traced()) return;
  _tb = rec._tb;
  _initiator = rec._emitter;
  _tb->begin(_initiator);
}

Recorder::Instruction::~Instruction() {
  // Non-null here means commit() never ran, so we are unwinding out of a half-executed instruction.
  if (_tb != nullptr) _tb->abort(_initiator);
}

void Recorder::Instruction::tick(i16 delta) {
  if (_tb == nullptr) return;
  // Resolve rather than using the Device::ID overloads, which assert on a closed recording and dereference null once
  // NDEBUG removes the assert.
  auto rec = _tb->find_recording(_initiator);
  if (rec == nullptr) return;
  // The immediate form carries its two payload bytes in the instruction stream. Reading from DP instead would be
  // smaller, but it would spend the data pointer that the body is about to set up for its own payloads.
  // This runs before any wrote(), so the tick lands at the head of the body, ahead of the DP steps.
  // The same instruction executing at different clock rates will now produce different deltas -- something we can
  // optimize for in the future.
  const auto isyn = tvm::EncodedOp::ISyn<1>{}.encode(static_cast<u16>(delta));
  _tb->emit_body(*rec, {isyn.data(), isyn.size()});
}

void Recorder::Instruction::commit() {
  if (_tb == nullptr) return;
  // Clear before committing: if commit() throws, the destructor must not then abort a recording that commit()
  // already closed.
  auto *tb = std::exchange(_tb, nullptr);
  tb->commit(_initiator);
}

void Recorder::set_traced(bool enabled) {
  if (_tb) _tb->trace(_emitter, enabled);
}

void Recorder::emit_write(const Operation &op, Address address, bits::span<const u8> prior, bits::span<const u8> now) {
  // Only pay the lambda/function ref cost when tracing is enabled.
  if (!traced()) return;
  const std::size_t len = std::min(prior.size(), now.size());
  emit_write(op, address, now.first(len), [&](bits::span<u8> tmp) { bits::memcpy(tmp, prior.first(len)); });
}

void Recorder::emit_write_increment(const Operation &op, Address address, bits::span<const u8> prior,
                                    bits::span<const u8> now, bits::Order order) {
  const std::size_t len = now.size();
  if (len == 0) return;       // A write with no data is meaningless.
  else if (!traced()) return; // Don't record for untraced.
  else if (op.type == Operation::Type::BufferInternal)
    return; // Access related to TB or UI. Filter or we'll loop infinitely.
  // The delta is computed in a u64, and the packet's payload words are sized at compile time, so only the widths
  // switched over below can take this form. Anything else still has to be recorded or the trace stops being
  // reversible, so hand it to the XOR encoding, which is width-agnostic.
  else if (len != 1 && len != 2 && len != 4 && len != 8) return emit_write(op, address, prior, now);
  const auto rec = _tb->find_recording(op.initiator);
  // begin() never called for that initiator.
  if (rec == nullptr) return;

  // Read the bytes from their target order into the host order.
  const u64 before = bits::memcpy_endian<u64>(prior, order);
  const u64 after = bits::memcpy_endian<u64>(now, order);
  const u64 delta = after - before;

  const auto off = tvm::SegmentPair{.hi = (u16)(address >> 16), .lo = (u16)(address & 0xFFFF)};
  // STEPMEM reads one size for both the delta and the destination, so the payload is emitted at the width of the
  // write. TraceBuffer::address_in_payload has no say here: there is no D variant of this opcode, and the caller
  // asking for a step is asking for a body that stands alone.
  const auto emit = [&]<std::size_t N>() {
    std::array<u8, N> payload{};
    bits::memcpy_endian(bits::span<u8>{payload.data(), N}, bits::Order::LittleEndian, delta);
    const auto step =
        tvm::EncodedOp::StepMem<6>(op.as_u16(), _emitter.value, off, tvm::encode_order(order)).encode(payload);
    _tb->emit_body(*rec, {step.data(), step.size()});
  };
  switch (len) {
  case 1: emit.operator()<1>(); break;
  case 2: emit.operator()<2>(); break;
  case 4: emit.operator()<4>(); break;
  case 8: emit.operator()<8>(); break;
  default: break; // Unreachable: the width guard above admits nothing else.
  }
  // Deliberately no emit_dp_update and no append_data. An immediate payload leaves DP and DS exactly as they were,
  // which is also what lets a following emit_write in the same recording step from the anchor it expects.
}

void Recorder::emit_write(const Operation &op, Address address, bits::span<const u8> now, PriorFiller fill_prior) {
  const std::size_t len = now.size();
  if (len == 0) return;       // A write with no data is meaningless.
  else if (!traced()) return; // Don't record for untraced.
  else if (op.type == Operation::Type::BufferInternal)
    return; // Access related to TB or UI. Filter or we'll loop infinitely.
  const auto rec = _tb->find_recording(op.initiator);
  // begin() never called for that initiator.
  if (rec == nullptr) return;

  // If using SETMEMDX encoding, we need to reserve additional bytes for the OFFSET value.
  const bool address_in_payload = _tb->address_in_payload(_emitter);
  const std::size_t prologue = address_in_payload ? tvm::SETMEMDX_ADDRESS_BYTES : 0;
  const auto slot = _tb->append_data_uninitialized(*rec, prologue + len);
  if (address_in_payload) {
    // OFF.hi then OFF.lo, each a little-endian 16-bit word -- the layout decode_setmemdx reads back.
    slot.bytes[0] = (u8)((address >> 16) & 0xFF);
    slot.bytes[1] = (u8)((address >> 24) & 0xFF);
    slot.bytes[2] = (u8)((address >> 0) & 0xFF);
    slot.bytes[3] = (u8)((address >> 8) & 0xFF);
  }
  // First 4 bytes of our allocation was an address. Use remaining bytes for the actual data value.
  const auto payload = slot.bytes.subspan(prologue);
  fill_prior(payload);
  // Always use now ^ old encoded payloads, since that operation is its own inverse.
  bits::inplace_xor(payload, now);

  emit_dp_update(slot, *rec, (u16)len, (u16)prologue);

  if (address_in_payload) {
    // No address or data in instruction, which increases opportunities for stencil dedup.
    const auto set = tvm::EncodedOp::SetMemDX<2>{.access = op.as_u16(), .dev = _emitter.value}.encode();
    _tb->emit_body(*rec, {set.data(), set.size()});
  } else {
    const auto off = tvm::SegmentPair{.hi = (u16)(address >> 16), .lo = (u16)(address & 0xFFFF)};
    const auto set = tvm::EncodedOp::SetMem<true, 4>{.access = op.as_u16(), .dev = _emitter.value, .off = off}.encode();
    _tb->emit_body(*rec, {set.data(), set.size()});
  }
}

void Recorder::emit_mm_write(const Operation &op, Address address, u8 pushed) {
  return emit_mm(op, address, pushed, true);
}

void Recorder::emit_mm_read(const Operation &op, Address address, u8 popped) {
  return emit_mm(op, address, popped, false);
}

void Recorder::emit_incr_register(const Operation &op, RegisterScan::RegisterRef ref, i16 value) {
  if (value == 0) return;     // A step of nothing replays to nothing in either direction.
  else if (!traced()) return; // Don't record for untraced.
  else if (op.type == Operation::Type::BufferInternal)
    return; // Access related to TB or UI. Filter or we'll loop infinitely.
  // Register 0 is never handed out by RegisterScan::expose, so this is a device that never exposed the counter it is
  // trying to step. Drop it here rather than letting the replay hard-stop on RegisterInvalid.
  else if (ref.reg.value == 0) return;
  const auto rec = _tb->find_recording(op.initiator);
  // begin() never called for that initiator.
  if (rec == nullptr) return;

  // Nothing is appended and no DP update is emitted: the payload is immediate, so DP and DS come out as they went in.
  // The register's width and byte order stay STEPREG's business, since the scan reports both -- which is why the
  // delta may be narrower than the counter it steps.
  const auto emit = [&](auto payload) {
    const auto step = tvm::EncodedOp::StepReg<4>(op.as_u16(), ref.reg.value, ref.field.value).encode(payload);
    _tb->emit_body(*rec, {step.data(), step.size()});
  };
  // Payloads are little-endian and signed. One byte covers the +-1 steps this exists for; the rest take two.
  if (value >= -128 && value <= 127) emit(std::array<u8, 1>{(u8)value});
  else emit(std::array<u8, 2>{(u8)(value & 0xFF), (u8)((value >> 8) & 0xFF)});
}

void Recorder::emit_register_xor(const Operation &op, RegisterScan::RegisterRef ref, u64 combined, u8 size) {
  if (combined == 0) return;  // The write changed nothing, so there is nothing to undo.
  else if (!traced()) return; // Don't record for untraced.
  else if (op.type == Operation::Type::BufferInternal)
    return; // Access related to TB or UI. Filter or we'll loop infinitely.
  // Register 0 is never handed out by RegisterScan::expose, so this is a device that never exposed the register it is
  // trying to write. Drop it here rather than letting the replay hard-stop on RegisterInvalid.
  else if (ref.reg.value == 0) return;
  const auto rec = _tb->find_recording(op.initiator);
  // begin() never called for that initiator.
  if (rec == nullptr) return;

  // Payload goes in the data chain and the instruction reaches it through DP, exactly as SETMEMX does.
  // Generic register writes don't have a discernable pattern, so emitting immediate data would supress stencil
  // promotion.
  const auto slot = _tb->append_data_uninitialized(*rec, size);
  // Immediates and payloads are little-endian throughout this ISA.
  bits::memcpy_endian(slot.bytes, bits::Order::LittleEndian, combined);
  emit_dp_update(slot, *rec, size, 0);

  const auto set =
      tvm::EncodedOp::SetReg<true, 3>{.access = op.as_u16(), .reg = ref.reg.value, .field = ref.field.value}.encode();
  _tb->emit_body(*rec, {set.data(), set.size()});
}

void Recorder::emit_mm(const Operation &op, Address address, u8 pushed, bool read_write) {
  static constexpr u16 len = 1; // MMIO data payload is one byte.
  if (!traced()) return;        // Don't record for untraced.
  else if (op.type == Operation::Type::BufferInternal)
    return; // Access related to TB or UI. Filter or we'll loop infinitely.
  const auto rec = _tb->find_recording(op.initiator);
  // begin() never called for that initiator.
  if (rec == nullptr) return;
  const auto slot = _tb->append_data_uninitialized(*rec, tvm::MMIO_PROLOGUE_BYTES + len);
  // OFF.hi then OFF.lo, each a little-endian 16-bit word.
  slot.bytes[0] = (u8)((address >> 16) & 0xFF);
  slot.bytes[1] = (u8)((address >> 24) & 0xFF);
  slot.bytes[2] = (u8)((address >> 0) & 0xFF);
  slot.bytes[3] = (u8)((address >> 8) & 0xFF);
  // Followed by the data value read or written.
  slot.bytes[4] = pushed;
  emit_dp_update(slot, *rec, 1, tvm::MMIO_PROLOGUE_BYTES);
  // Emit the actual MMIO instruction after the DP update instruction.
  const auto mmio =
      tvm::EncodedOp::MMIO<3>{.read_write = read_write, .access = op.as_u16(), .dev = _emitter.value}.encode();
  _tb->emit_body(*rec, {mmio.data(), mmio.size()});
}

void Recorder::emit_dp_update(const tvm::DataSlot &slot, tvm::Recording &rec, u16 len, u16 prologue) {
  // Update DP to point at our data body, using the cheapest/smallest encoding possible.
  const auto anchor = _tb->dp_anchor(rec);
  if (!anchor.set) {
    // This is the first data payload of this instruction. DP will be preloaded from the location buffer via
    // run_each, but DS is left unset/0. So set DS to this payload's size
    const auto set_ds = tvm::EncodedOp::LDR<tvm::RegMask::DS>{(u16)len}.encode();
    _tb->emit_body(rec, {set_ds.data(), set_ds.size()});
  } else if (anchor.at.id != slot.loc.id) {
    // The data chain rolled onto a new buffer mid-recording, which means mid-body update to DP.
    const auto ldp =
        tvm::EncodedOp::LDP<3>{tvm::SegmentPair{.hi = slot.loc.id.value, .lo = slot.loc.offset}, (u16)len}.encode();
    _tb->emit_body(rec, {ldp.data(), ldp.size()});
  } else if (anchor.stride == anchor.size && slot.loc.offset == (u16)(anchor.at.offset + anchor.stride)) {
    // Packed directly after the previous record, which is what happens when one initiator writes several times in a
    // row. ACCDP advances DP by the *previous* DS, so this encodes in 4 bytes and carries no absolute address --
    // making it identical across programs that touch the same things.
    //
    // Only valid when that previous record was exactly its payload. If the allocation != DS, we have to choose INCDP
    // with the explicit DP increment.
    const auto accdp = tvm::EncodedOp::ACCDP{(u16)len}.encode();
    _tb->emit_body(rec, {accdp.data(), accdp.size()});
  } else {
    // The step is explicit: either the previous record carried a prologue, or something sits between the two. Still
    // a constant for a given instruction shape, so it does not by itself spoil de-duplication.
    const auto incdp = tvm::EncodedOp::INCDP{(u16)(slot.loc.offset - anchor.at.offset), (u16)len}.encode();
    _tb->emit_body(rec, {incdp.data(), incdp.size()});
  }
  _tb->set_dp_anchor(rec, slot.loc, (u16)len, (u16)(prologue + len));
}

} // namespace trace
