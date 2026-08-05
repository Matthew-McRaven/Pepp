#pragma once
#include <array>
#include "core/ds/alloc/pagechain.hpp"
#include "core/integers.h"
#include "core/sim/debugger/tvm_opcodes.hpp"

namespace tvm {

// Where one recorded program lives: its entry point, and where its data payload begins.
// It is more efficient to encode the data payload here than to insert it in a programs prefix.
// Here it costs 4 bytes, but an LDP costs 8. Every program will need data.
//
// A record with no payload leaves `data` null. Buffer::ID{0} is never a valid buffer, so that is unambiguous.
struct ProgramLocation {
  pepp::bts::Buffer::Location code{};
  pepp::bts::Buffer::Location data{};
};

// Which registers survive when a program restarts. See MachineState::restart.
enum class RegisterRetention : u8 {
  None = 0, // All registers are reset
  DP = 1,   // DS and DP registers
  All = 2,  // No registers are reset
};

// Condition/status bits. Shared by the decoder and every backend: decode sets TR, CLRMOD and the MOD-enable bits,
// while backends set N/Z from comparisons and F from access outcomes.
struct Flags {
  Flags() = default;
  Flags(u8 v)
      : N(v & 0x01), Z((v >> 1) & 0x01), TR((v >> 2) & 0x01), L((v >> 3) & 0x01), M1((v >> 4) & 0x01),
        M2((v >> 5) & 0x01), CLRMOD((v >> 6) & 0x01), F((v >> 7) & 0x01) {}
  // Result of the last comparison was negative
  u8 N : 1 = 0;
  // Result of the last comparison was zero
  u8 Z : 1 = 0;
  // If 0, in target mode / id contains a target id
  // if 1, in register mode / id containrs a register+field ID.
  u8 TR : 1 = 0;
  // Live bit. If 0, the blaster is halted. If 1, the blaster is running.
  // NOTE: the default of 1 is load-bearing -- MachineState::restart relies on `Flags{}` coming back live.
  u8 L : 1 = 1;
  // 1 if M1/M2 are currently enabled, 0 otherwise. When setting M1 or M2 to 0, the associated register should be
  // cleared to. Access to a disabled register must not fault.
  u8 M1 : 1 = 0;
  u8 M2 : 1 = 0;
  // If 1, clear MOD1/MOD2 at the start of the next instruction.
  u8 CLRMOD : 1 = 0;
  // Set to 1 if the last memory access failed. The blaster will not halt, but someone should check this flag!
  u8 F : 1 = 0;
  u8 as_u8() const {
    return (N << 0) | (Z << 1) | (TR << 2) | (L << 3) | (M1 << 4) | (M2 << 5) | (CLRMOD << 6) | (F << 7);
  }
};

// The architectural register file. Decode programs most of these as a side effect of reading an instruction packet,
// which is what makes register retention work; backends read them back and may write IP/DP/SP.
struct Registers {
  // Contain the 2-byte aligned instruction pointer. hi contains buffer ptr, lo contains offset.
  tvm::SegmentPair IP{};
  // Some instructions  process data. hi contains that data's buffer ptr, and lo is an offset into that buffer.
  tvm::SegmentPair DP{};
  // Length of data at DP in bytes
  u16 DS = 0;
  // Contains the access kind for next memory access
  u16 ACCESS = 0;
  // If in target mode, lo contains the 16-bit ID of a target device
  // If in register mode, hi is the 16-bit register ID and lo is 16-bit field ID.
  tvm::SegmentPair ID = {};
  // If in target mode, hi contains the 16-bit offset into the target's address space, and lo contains the 16-bit
  // offset. If in register mode, unused.
  tvm::SegmentPair OFF = {};
  // Modifiers register, whose meaning depends on the instruction being executed
  tvm::SegmentPair MOD1 = {}, MOD2 = {};
  /*
   * None of the following registers are accessible vis load-masked-register
   */
  // When the machine is stopped, indicated the reason why.
  // If None, then the machine is either running or stopped normally and can be resumed easily.
  // If set to a cause, you need to address the underlying reason before resuming.
  tvm::StopCause STOP_CAUSE = tvm::StopCause::None;
  // Stack pointer, used to make call/ret work.
  u16 SP = 0;
  // 16-bit integer which contains a decoded instruction.
  tvm::OpWord IS{};
};

// Everything a running program can observe or modify, with no opinion about what an instruction *means*.
//
// This exists so that one decoder can feed several backends. The decoder programs registers as it cracks a packet,
// and the backend reads them back and applies whatever that particular backend does -- writing the real machine,
// recording which locations were touched, folding deltas together. All of them need the same registers, flags, and
// call stack, so those live here rather than in either half.
class MachineState {
public:
  Flags csrs{};
  Registers regs{};

  // Halt with L==0, F==0. A program that ran to its HALT ends this way.
  void soft_stop(tvm::StopCause cause = tvm::StopCause::None);
  // Halt with L==0, F==1. Signals the driver that continuing to the next program is pointless.
  void hard_stop(tvm::StopCause cause = tvm::StopCause::None);
  bool stopped() const { return csrs.L == 0; }
  tvm::StopCause stop_cause() const { return regs.STOP_CAUSE; }

  // Reset per `retain`, then bring the machine back to live (L==1) with the previous failure cleared (F==0).
  void restart(RegisterRetention retain);

  void update_ip(pepp::bts::Buffer::Location loc);
  void update_ip(pepp::bts::Buffer::ID id, u16 offset = 0);
  // Point DP at `loc`. Does not touch DS: a program's first payload width is a property of its instruction shape,
  // not of this execution, so it is stated in the body where it templates away rather than carried per program.
  void update_dp(pepp::bts::Buffer::Location loc);

  // SP += 4 and write the 4 bytes. Soft-stops with StackOverflow if the stack is full.
  void push(tvm::SegmentPair v);
  // SP -= 4 and return the 4 bytes. Soft-stops with StackUnderflow and returns {} if the stack is empty.
  tvm::SegmentPair pop();

private:
  std::array<u8, 256> _stack{};
};

// Little-endian 2-byte read out of a buffer. Hard-stops with InvalidIBuffer and returns 0 when the buffer is unknown.
// Shared because both the decoder (fetching instruction words) and backends (reading immediate register data) need it.
u16 read16(pepp::bts::BufferManager &mgr, MachineState &state, pepp::bts::Buffer::ID id, u16 offset);

} // namespace tvm
