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
  // Clear before committing: if commit() throws RingOverflow, the destructor must not then abort a recording that
  // commit() already closed.
  auto *tb = std::exchange(_tb, nullptr);
  tb->commit(_initiator);
}

void Recorder::set_traced(bool enabled) {
  if (_tb) _tb->trace(_emitter, enabled);
}

void Recorder::emit_write(const Operation &op, Address address, bits::span<const u8> old, bits::span<const u8> now) {
  // Only pay the lambda/function ref cost when tracing is enabled.
  if (!traced()) return;
  const std::size_t len = std::min(old.size(), now.size());
  emit_write(op, address, now.first(len), [&](bits::span<u8> prior) { bits::memcpy(prior, old.first(len)); });
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

  // Always use now ^ old encoded payloads, since that operation is its own inverse.
  // Use unitialized memory so we don't have to allocate separate scratch space.
  const auto slot = _tb->append_data_uninitialized(*rec, len);
  fill_prior(slot.bytes);
  bits::inplace_xor(slot.bytes, now);

  // Update DP to point at our data body, using the cheapest/smallest encoding possible.
  const auto anchor = _tb->dp_anchor(*rec);
  if (!anchor.set) {
    // First DP-relative instruction of this recording. Perform a full LDP load in the prefix so all future ops this
    // packet can use a delta.
    const auto ldp =
        tvm::EncodedOp::LDP<3>{tvm::SegmentPair{.hi = slot.loc.id.value, .lo = slot.loc.offset}, (u16)len}.encode();
    _tb->emit_prefix(*rec, {ldp.data(), ldp.size()});
  } else if (anchor.at.id != slot.loc.id) {
    // The data chain rolled onto a new buffer mid-recording, which means mid-body update to DP.
    const auto ldp =
        tvm::EncodedOp::LDP<3>{tvm::SegmentPair{.hi = slot.loc.id.value, .lo = slot.loc.offset}, (u16)len}.encode();
    _tb->emit_body(*rec, {ldp.data(), ldp.size()});
  } else if (slot.loc.offset == (u16)(anchor.at.offset + anchor.size)) {
    // Packed directly after the previous payload, which is what happens when one initiator writes several times in a
    // row. ACCDP advances DP by the *previous* DS, so this encodes in 4 bytes and carries no absolute address.
    const auto accdp = tvm::EncodedOp::ACCDP{(u16)len}.encode();
    _tb->emit_body(*rec, {accdp.data(), accdp.size()});
  } else {
    // Another initiator interleaved its data between our two payloads, so the step is explicit.
    const auto incdp = tvm::EncodedOp::INCDP{(u16)(slot.loc.offset - anchor.at.offset), (u16)len}.encode();
    _tb->emit_body(*rec, {incdp.data(), incdp.size()});
  }
  _tb->set_dp_anchor(*rec, slot.loc, (u16)len);

  const auto off = tvm::SegmentPair{.hi = (u16)(address >> 16), .lo = (u16)(address & 0xFFFF)};
  const auto set = tvm::EncodedOp::SetMem<true, 4>{.access = op.as_u16(), .dev = _emitter.value, .off = off}.encode();
  _tb->emit_body(*rec, {set.data(), set.size()});
}

} // namespace trace
