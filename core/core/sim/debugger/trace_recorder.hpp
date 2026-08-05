#pragma once
#include <concepts>
#include <memory>
#include <type_traits>
#include "core/integers.h"
#include "core/math/bitmanip/span.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"

namespace tvm {
class TraceBuffer;
}

namespace trace {

// A cheaply copyable class which provides convenient methods to write to a TraceBuffer.
// It is bound to a particular device ID, and this recorder discards writes if that device is not actively traced.
// It also standardizes discarding traces for Application and BufferInternal accesses.
//
// Devices must report an access BEFORE it occurs (write-ahead-log), so that the trace machinery can preserve the
// previous bytes before they are overwritten. Devices must not throw between the time they've created the trace but
// before the update is externally visible from the device.
//
// All forms of filtering occur here rather than at the call site, including:
//   - this recorder is unbound / TB is nullptr.
//   - this device not being traced.
//   - BufferInternal accesses, which are the replay machinery reading and writing through the same paths
//   - accesses that arrive with no recording begun for the Operation::initiator
class Recorder {
public:
  // Span size is a proxy for "do this device's addresses repeat", and a crude one. It wants to become an explicit
  // property of the device rather than a threshold guessed here.
  // A target whose address space is at most this many bytes is assumed to write the same handful of offsets over and
  // over (a register bank, a CSR block)  and keeps its offsets in the instruction packet. Anything larger is
  // assumed to write a different address almost every time and moves them into the payload instead.
  static constexpr std::size_t NARROW_TARGET_BYTES = 256;

  // Default-constructed recorders are inert, which is what an untraced device holds.
  Recorder() = default;
  // `emitter` is the device that owns the bytes -- the target a replayed write is aimed at.
  //
  // `address_in_payload` selects how a write's target offset is encoded.
  // When false, we prefer packet formats which encode offsets in the instruction,
  // When true, we prefer packet formats which encode offsets in the datastream.
  // If you have a consistent access pattern to offsets (register banks), set to false. ALl other cases should be true.
  // Moving the address into the payload makes updating the offset more expensive but provides more opportunities for
  // de-duplication. Not all operations support both formats, in which case this recorder will choose whichever is
  // available
  Recorder(tvm::TraceBuffer *tb, Device::ID emitter, bool address_in_payload = false)
      : _tb(tb), _emitter(emitter), _address_in_payload(address_in_payload) {}

  // Whether this device's writes are being recorded.
  bool traced() const;
  // Switch recording of this device on or off.
  void set_traced(bool enabled);
  Device::ID emitter() const { return _emitter; }

  // Non-owning reference to a `void(bits::span<u8>)` callable. This is effectively std::function_ref from C++26.
  // It is prone to UB since it is a function ref. Avoid the following pattern:
  //    PriorFiller pf = [](bits::span<u8>){};  // temporary lambda destroyed after this line
  //    pf(out);                                // UB: _obj dangles
  //
  // std::function would allocate on a path that runs for every traced memory access, which function ref makes cheaper.
  class PriorFiller {
  public:
    template <typename F>
      requires std::invocable<F &, bits::span<u8>> && (!std::same_as<std::remove_cvref_t<F>, PriorFiller>)
    PriorFiller(F &&fn)
        : _obj(std::addressof(fn)),
          _shim([](void *o, bits::span<u8> out) { (*static_cast<std::remove_reference_t<F> *>(o))(out); }) {}
    void operator()(bits::span<u8> out) const { _shim(_obj, out); }

  private:
    void *_obj;
    void (*_shim)(void *, bits::span<u8>);
  };

  // Record a pending write of `now` over `old` at `address`.
  //
  // The two spans are folded to `old ^ now` and replayed by SETMEMX, which does a read-XOR-write. That makes one
  // record serve both directions: applied to `old` it yields `now`, applied to `now` it yields `old` back. If the
  // spans differ in length the shorter one wins.
  //
  // Use this when the previous bytes are already materialized, as they are for a device backed by a flat array.
  //
  // The absolute data-pointer setup goes into the recording's prefix and later writes in the same recording use a
  // delta, so that bodies stay free of per-instance addresses and can be de-duplicated into the template chain.
  void emit_write(const Operation &op, Address address, bits::span<const u8> old, bits::span<const u8> now);

  // Same, for a device whose previous contents are not sitting in memory ready to read -- a paged pool, a cache, a
  // banked memory. `fill_prior` is handed a span of exactly now.size() bytes *inside the trace record* and must write
  // the previous contents into it; there is no scratch buffer anywhere in the path.
  //
  // Note the argument order: `now` takes the slot `old` occupies above, because there is no `old` to pass -- that is
  // what fill_prior is for.
  //
  // fill_prior is not invoked when the write is not being recorded, so an untraced device pays nothing for a fetch
  // that would have been thrown away.
  void emit_write(const Operation &op, Address address, bits::span<const u8> now, PriorFiller fill_prior);

  // A helper class which which helps open & close a recording for a single instruction.
  class Instruction {
  public:
    // Opens a recording when `rec` is bound and traced, and is inert otherwise -- so an untraced CPU pays one bitset
    // test per instruction and nothing more.
    explicit Instruction(const Recorder &rec);
    // If commit() was never called, abort the recording rather than commit(), since commit can throw RingOverflow.
    // So commit() is the happy path, and this is the failure path.
    ~Instruction();
    Instruction(const Instruction &) = delete;
    Instruction &operator=(const Instruction &) = delete;
    Instruction(Instruction &&) = delete;
    Instruction &operator=(Instruction &&) = delete;

    // Record an ISYN with a relative tick count, which is the number of ticks since the PREVIOUS GLOBAL TICK FOR THAT
    // CLOCK. This value will be provided to you by the caller of our tick() equivalent. The value is carried as two
    // bytes and sign-extended on decode, so it spans the same range as i16.
    void tick(i16 delta);

    // Finish the record. A no-op when nothing was opened. Throws tvm::RingOverflow if the ring will become fully
    void commit();

    // True when a recording was actually opened.
    explicit operator bool() const { return _tb != nullptr; }

  private:
    tvm::TraceBuffer *_tb = nullptr;
    Device::ID _initiator{};
  };

private:
  // Null means this device was never bound to a buffer.
  tvm::TraceBuffer *_tb = nullptr;
  Device::ID _emitter{};
  // If true, prefer instruction variants which encode their target offset as data rather than in the instruction.
  bool _address_in_payload = false;
};

} // namespace trace
