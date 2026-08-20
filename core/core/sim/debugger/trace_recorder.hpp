#pragma once
#include <concepts>
#include <memory>
#include <type_traits>
#include "core/integers.h"
#include "core/math/bitmanip/span.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"
// For RegisterScan::RegisterRef, which emit_incr_register takes by value. It is a pair of 16-bit handles, and being a
// nested type there is no forward declaring it.
#include "core/sim/debugger/register_scanner.hpp"

namespace tvm {
class DataSlot;
class TraceBuffer;
class Recording;
} // namespace tvm

namespace trace {

// A cheaply copyable class which provides convenient methods to write to a TraceBuffer.
// It is bound to a particular device ID, and this recorder discards writes if that device is not actively traced.
// It also standardizes discarding traces for BufferInternal accesses.
//
// Devices must report an access BEFORE it occurs (write-ahead-log), so that the trace machinery can preserve the
// previous bytes before they are overwritten. Devices must not throw between the time they've created the trace but
// before the update is externally visible from the device.
//
// All forms of filtering occur here rather than at the call site, including:
//   - this recorder is unbound / TB is nullptr.
//   - this device not being traced.
//   - BufferInternal accesses, which are the replay machinery reading and writing through the same paths
//     Application accesses are not filtered, but will be discarded if the is no open recording.
//   - accesses that arrive with no recording begun for the Operation::initiator
class Recorder {
public:
  // Default-constructed recorders are inert, which is what an untraced device holds.
  Recorder() = default;
  // `emitter` is the device that owns the bytes -- the target a replayed write is aimed at.
  // How that device's writes encode their target offset is not settled here: it belongs to the buffer, alongside the
  // traced bit, so that there is one representation of it. See TraceBuffer::set_address_in_payload.
  Recorder(tvm::TraceBuffer *tb, Device::ID emitter) : _tb(tb), _emitter(emitter) {}

  // Whether this device's writes are being recorded. A device on a hot path should cache this value via
  // Traceable::on_traced_changed rather than ask per access.
  bool traced() const;
  // Switch recording of this device on or off.
  void set_traced(bool enabled);
  Device::ID emitter() const { return _emitter; }

  // Non-owning reference to a `void(bits::span<u8>)` callable. This is effectively std::function_ref from C++26.
  // It is prone to UB since it is a function ref. Avoid the following pattern:
  //    PriorFiller pf = [](bits::span<u8>){};  // temporary lambda destroyed after this line
  //    pf(out);                                // UB: _obj dangles
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

  // Record a pending write of now over old at address.
  //
  // The two spans are combined into the trace buffer as a single span old^now. We always choose a SETMEMX variant,
  // which allows the trace to be applied either forwards or backwards.  If the spans differ in length the shorter one
  // wins.
  //
  // Prefer this overload if you don't have to allocate a temp buffer to access the old value, such as with a Dense
  // device.
  void emit_write(const Operation &op, Address address, bits::span<const u8> prior, bits::span<const u8> now);
  // Record the same write as STEPMEM, whose payload is `now - prior` rather than `now ^ prior`.
  //
  // Both the address/offset and the delta are encoded in the instruction packet, which means this does not impact the
  // data chain. For memory locations which are updated by a fixed increment each instruction (program counter, cycle
  // counter), this produces a byte-identical instruction where a SETMEMX encoding would not.
  // For a per-cycle PC update, this saves ~2B per instruction, which is a 10% footprint savings.
  // In all other cases (writes to main-memory), I expect STEPMEM to be worse than SETMEMX because of the presence of
  // offset in the packet.
  //
  // Byte order matters when computing the difference (now - prior), which is why we require an explicit order. Writes
  // whose width is not 1, 2, 4, or 8 bytes fall back to emit_write.
  void emit_write_increment(const Operation &op, Address address, bits::span<const u8> prior, bits::span<const u8> now,
                            bits::Order order = bits::hostOrder());

  // Same, for a device whose previous contents are not sitting in contiguous memory, such as Sparse or a bus.
  // fill_prior is a function reference which writes the prior bytes into a provided span. That provided span is in the
  // trace buffer and is guaranteed to be the same length as now and it is contiguous.
  //
  // fill_prior is not invoked when the write is not being recorded, so an untraced device pays nothing for a fetch
  // that would have been thrown away beyond the cost of allocating a function ref.
  void emit_write(const Operation &op, Address address, bits::span<const u8> now, PriorFiller fill_prior);

  // Pushing a byte onto "output" side of memory-mapped FIFO.
  void emit_mm_write(const Operation &op, Address address, u8 pushed);
  // Popped a byte from the "input" side of the memory-mapped FIFO. Since pop from a FIFO is destructive, we must
  // record that value, otherwise we cannot undo this action.
  void emit_mm_read(const Operation &op, Address address, u8 popped);

  // Record a signed step of a scan-exposed register: the STEPREG sibling of emit_write_increment.
  //
  // The register reference and the delta both ride in the instruction packet, so this writes nothing to the data
  // chain and leaves DP and DS alone. A register incremented by a fixed amount for an instruction class will have a
  // byte-identical body, which promotes to a stencil and collapses the whole record to a CALL carrying no
  // payload at all.
  void emit_incr_register(const Operation &op, RegisterScan::RegisterRef ref, i16 value);

  // Record an overwrite of a scan-exposed register as SETREGX, whose payload is an already-combined `now ^ prior`.
  //
  // The width comes from I, and must match the register's declared byte_width. Unlike the STEPREG form, the payload
  // will be DP-relative rather than immediate to increase opportunities for stencil promotion.
  template <std::integral I> void emit_write_register(const Operation &op, RegisterScan::RegisterRef ref, I combined) {
    emit_register_xor(op, ref, static_cast<u64>(static_cast<std::make_unsigned_t<I>>(combined)), sizeof(I));
  }

  // A helper class which which helps open & close a recording for a single instruction.
  // Multiple methods are partially inlined. Allowing every TU to see tha guard condition has lead to substantially
  // faster code.
  class Instruction {
  public:
    // Opens a recording when `traced` says to, and is inert otherwise. The caller passes its own cached copy of the
    // bit rather than reaching into the TraceBuffer to avoid an expensive call for each instruction. If you don't have
    // a cached copy, you'll need to work with your recorder instead.
    explicit Instruction(const Recorder &rec, bool traced) {
      if (traced) open(rec);
    }
    // If commit() was never called, abort() the recording rather than commit(). Destructors run while an exception
    // unwinds, and commit() can throw, which would call std::terminate.
    ~Instruction() {
      if (_tb != nullptr) abort();
    }
    Instruction(const Instruction &) = delete;
    Instruction &operator=(const Instruction &) = delete;
    Instruction(Instruction &&) = delete;
    Instruction &operator=(Instruction &&) = delete;

    // Record an ISYN with a relative tick count, which is the number of ticks since the PREVIOUS GLOBAL TICK FOR THAT
    // CLOCK. This value will be provided to you by the caller of our tick() equivalent. The value is carried as two
    // bytes and sign-extended on decode, so it spans the same range as i16.
    void tick(i16 delta) {
      if (_tb != nullptr) tick_slow(delta);
    }

    // Finish the record. A no-op when nothing was opened.
    void commit() {
      if (_tb != nullptr) commit_slow();
    }

    // True when a recording was actually opened.
    explicit operator bool() const { return _tb != nullptr; }

  private:
    // Out of line because each needs the TraceBuffer, which this header only forward declares. Reached only when a
    // recording is actually open.
    void open(const Recorder &rec);
    void abort();
    void tick_slow(i16 delta);
    void commit_slow();

    tvm::TraceBuffer *_tb = nullptr;
    Device::ID _initiator{};
  };

private:
  // 0 if read, 1 if write.
  void emit_mm(const Operation &op, Address address, u8 pushed, bool read_write);
  // Width-erased body of emit_write_register. Out of line because it needs the TraceBuffer, which this header only
  // forward declares.
  void emit_register_xor(const Operation &op, RegisterScan::RegisterRef ref, u64 combined, u8 size);
  void emit_dp_update(const tvm::DataSlot &slot, tvm::Recording &rec, u16 size, u16 prologue);
  // Null means this device was never bound to a buffer.
  tvm::TraceBuffer *_tb = nullptr;
  Device::ID _emitter{};
};

} // namespace trace
