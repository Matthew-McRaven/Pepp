#include "core/sim/debugger/trace_recorder.hpp"
#include <algorithm>
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

  // Update DP to point at our data body, using the cheapest/smallest encoding possible.
  const auto anchor = _tb->dp_anchor(*rec);
  if (!anchor.set) {
    // This is the first data payload of this instruction. DP will be preloaded from the location buffer via
    // run_each, but DS is left unset/0. So set DS to this payload's size
    const auto set_ds = tvm::EncodedOp::LDR<tvm::RegMask::DS>{(u16)len}.encode();
    _tb->emit_body(*rec, {set_ds.data(), set_ds.size()});
  } else if (anchor.at.id != slot.loc.id) {
    // The data chain rolled onto a new buffer mid-recording, which means mid-body update to DP.
    const auto ldp =
        tvm::EncodedOp::LDP<3>{tvm::SegmentPair{.hi = slot.loc.id.value, .lo = slot.loc.offset}, (u16)len}.encode();
    _tb->emit_body(*rec, {ldp.data(), ldp.size()});
  } else if (anchor.stride == anchor.size && slot.loc.offset == (u16)(anchor.at.offset + anchor.stride)) {
    // Packed directly after the previous record, which is what happens when one initiator writes several times in a
    // row. ACCDP advances DP by the *previous* DS, so this encodes in 4 bytes and carries no absolute address --
    // making it identical across programs that touch the same things.
    //
    // Only valid when that previous record was exactly its payload. If the allocation != DS, we have to choose INCDP
    // with the explicit DP increment.
    const auto accdp = tvm::EncodedOp::ACCDP{(u16)len}.encode();
    _tb->emit_body(*rec, {accdp.data(), accdp.size()});
  } else {
    // The step is explicit: either the previous record carried a prologue, or something sits between the two. Still
    // a constant for a given instruction shape, so it does not by itself spoil de-duplication.
    const auto incdp = tvm::EncodedOp::INCDP{(u16)(slot.loc.offset - anchor.at.offset), (u16)len}.encode();
    _tb->emit_body(*rec, {incdp.data(), incdp.size()});
  }
  _tb->set_dp_anchor(*rec, slot.loc, (u16)len, (u16)(prologue + len));

  if (address_in_payload) {
    // No address or data in instruction, which increase opportunities for templatization.
    const auto set = tvm::EncodedOp::SetMemDX<2>{.access = op.as_u16(), .dev = _emitter.value}.encode();
    _tb->emit_body(*rec, {set.data(), set.size()});
  } else {
    const auto off = tvm::SegmentPair{.hi = (u16)(address >> 16), .lo = (u16)(address & 0xFFFF)};
    const auto set = tvm::EncodedOp::SetMem<true, 4>{.access = op.as_u16(), .dev = _emitter.value, .off = off}.encode();
    _tb->emit_body(*rec, {set.data(), set.size()});
  }
}

} // namespace trace
