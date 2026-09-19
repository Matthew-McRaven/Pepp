#pragma once
#include "core/integers.h"
#include "core/math/bitmanip/order.hpp"

// The system class from core/sim/system.hpp
class System;
class RegisterScan;

namespace tvm {
// Reasons that a TVM stopped.
enum class StopCause {
  None = 0,
  StackOverflow,
  StackUnderflow,
  IllegalOpcode,
  InvalidIBuffer,
  InvalidDBuffer,
  WrongTR,
  MissingSystem,
  RegisterInvalid,
  RegisterSizeMismatch,
  RegisterWidthIllegal,
  // Step* operations perform arithmetic as an i64. If the target is wider than 8 bytes, we don't know which bytes to
  // touch.
  StepWidthIllegal,
  TargetInvalid,
  TargetNotMemory,
  // Can't apply MMIO opcode to a non-FIFO target.
  TargetNotFIFO,
  // An INVRET with no matching INVCALL, or a program that reached HALT while still inside one. Either way the
  // direction counter no longer describes reality, so continuing would silently replay ops the wrong way round.
  UnbalancedInvCall,
  // A legal opcode this backend does not implement. Distinct from IllegalOpcode, which means the decoder did not
  // recognise the encoding at all: this one says the program is well-formed but aimed at the wrong backend.
  Unimplemented,
  // An op with no inverse executed during backwards replay. Undoing it is not possible.
  NotInvertible,
  // A load was aimed at a device which exists, but which cannot be loaded into.
  TargetNotLoadable,
  // A load named a (file, segment) pair which was never registered with the backend.
  SegmentUnknown,
  // An access to target or register failed.
  AccessRefused,
  // A STCALL with an invalid index.
  StencilUnknown,
};

// How a payload of a SET* operation combines with what is already at the destination.
enum class Delta : u8 {
  // Overwrite the destination with the payload. See: SETMEM, SETREG.
  Assign,
  // Read-xor-write, which has the benefit of being its own inverse. See: SETMEMX, SETREGX.
  Xor,
  // Read-add-write, with the payload to the destination as a signed integer, subtracting instead on a backward replay.
  // STEP*.
  Add,
};

// STEP*'s order word (MOD1.hi): set says the destination is big-endian, clear says little-endian.
constexpr u16 encode_order(bits::Order order) { return order == bits::Order::BigEndian ? 1 : 0; }
constexpr bits::Order decode_order(u16 order_word) {
  return order_word ? bits::Order::BigEndian : bits::Order::LittleEndian;
}

// An OpWord is CLRMOD in bit 15 above a 15-bit opcode, whose top two bits select its class:
//   00: fixed. All 13 remaining bits name the opcode. Length is solely determined by opcode.
//   10: variable. Bits 12..8 name the opcode, and the low byte is the packet length in words.
//   01, 11: reserved.
inline constexpr u16 OPPLANE_MASK = 0x6000;
inline constexpr u16 OPPLANE_FIXED = 0x0000;
inline constexpr u16 OPPLANE_VARIABLE = 0x4000;
inline constexpr u16 OPCODE_MASK = 0x7FFF;
inline constexpr u16 VARIABLE_OPCODE_MASK = 0x7F00;
// Distinguishes a near CALL/INVCALL/branch/BRF from its far form, and HALTC from HALT.
inline constexpr u16 NEAR_MASK = 0x0100;

enum class Opcode : u16 {
  // Set L flag to 0, halting the blaster and F to 0. MOD1 is cause, MOD2 is ignored.
  // If the machine is halted and L==0 and F==1, it "hard stop". L==0 and F==0 is a "soft stop".
  // No packet registers.
  HALT = 0x0000,
  // HALT with cause
  // Packet registers: MOD1.lo
  HALTC = 0x0100,
  // Pop IP from SP. MOD1 and MOD2 are ignored
  RET = 0x0001,
  // Push next IP onto SP. Set IP.lo to MOD2.lo and IP.hi to MOD2.hi.
  // Packet registers: MOD2.lo, MOD2.hi
  CALL = 0x0002,
  // Near form of CALL, whose target is in the current buffer.
  // Packet registers: MOD2.lo
  CALLN = 0x0102,
  // A far CALL whose return address is a HALT rather than the next instruction, so that a program ending in a call
  // needs no HALT of its own. Where that HALT lives is up to the backend, which refuses the op if it has none.
  // Packet registers: MOD2.lo, MOD2.hi
  CALLHALT = 0x0003,
  // Call a stencil by its index rather than its address. The backend resolves the index and updates IP accordingly.
  // This saves 2B per instruction that uses a stencil. STCALLHALT is a variant whci returns to a HALT, like CALLHALT.
  // Packet registers: (stencil index)
  STCALL = 0x0202,
  STCALLHALT = 0x0203,
  // Synchronize absolute and synchronize incremental, which both take a timestamp / clock tick.
  // ASYN reports the full timestamp, whereas ISYN reports a signed delta to be added to the previous timestamp.
  // The two differ only in LSB, which is set for the incremental variant.
  // Data is located at DP/DS; see ASYNI/ISYNI for the immediate forms.
  // The data is a little-endian integer. A timestamp can't exceed 64 bits, so the resulting size will be clipped to 8
  // bytes, regardless of data source. The blaster does not retain a timestamp, so this value is purely for higher-level
  // analysis code.
  // No packet registers.
  ASYN = 0x0004,
  ISYN = 0x0005,
  // An invertible call, which is the escape hatch that lets an one-way operation participate in reverse replay.
  // Targets are picked on the replay direction, which is the forward target when stepping forward, and the backward
  // target when stepping backward. One of the two is always called.
  //
  // Everything reached through an INVCALL is treated as-if forward, even if the caller is in a backwards direction.
  // For an uninvertible op (CLRMEM), you could wrap it with an invcall. The clear is forward, and the backward
  // portion would restore the values prior to clear. This might not be cheap, but it is possible. Both call targets
  // must terminate in an explicit INVRET; we don't "guess" which RETs match an INVCALL. Ordinary CALLs/RETs work as
  // expected inside a INVCALL subroutine.
  //
  // The two targets are interleaved lo-first, so INVCALLN (both targets in the current buffer) is INVCALL's first
  // 2 words.
  // MOD1 holds the forward target, MOD2 holds the backward target.
  // Packet registers: (forward).lo, (backward).lo, (forward).hi, (backward).hi
  INVCALL = 0x0006,
  // Packet registers: (forward).lo, (backward).lo
  INVCALLN = 0x0106,
  // Branch if F bit is set, using the same packet registers are comparison branches.
  // Sets MOD1.lo to ConditionCode::F.
  // Packet registers: MOD2.lo, MOD2.hi
  BRF = 0x0007,
  // All branch instructions, with bit pattern  01 0lge. It's a bit of a psychotic encoding, allowing you to select
  // between 3 conditions simultaneously: (l) less than, (g) greater than, and (e) equal. This bits are sufficient to
  // synthesize all meaningful branch conditions, plus an uncondtional branch and noop.
  // Condition is encoded in opcode bits rather than a packet word to save space.
  // MOD1 will always be set to the condition code bits (bge). MOD2 is the displacement added to the IP if the branch
  // is taken. Reverse typical hi/lo so that a near branch is the first word of a far one.
  // Packet registers: MOD2.lo, MOD2.hi
  NOP = 0x0008,
  BREQ = 0x0009,
  BRGT = 0x000A,
  BRGE = 0x000B,
  BRLT = 0x000C,
  BRLE = 0x000D,
  BRNE = 0x000E,
  BR = 0x000F,
  // Near forms of the branches and BRF, which stay in the current buffer. Set NEAR_BIT on the far opcode.
  // Packet registers: MOD2.lo
  BRFN = 0x0107,
  NOPN = 0x0108,
  BREQN = 0x0109,
  BRGTN = 0x010A,
  BRGEN = 0x010B,
  BRLTN = 0x010C,
  BRLEN = 0x010D,
  BRNEN = 0x010E,
  BRN = 0x010F,
  // From here we begin memory / register operations. Memory operation act directly on a Target*, whereas Register
  // operations operate on a RegisterRef from a RegisterScan. While the encoding bits interleave mem/reg ops, they are
  // enumerated separately because they have different semantics.
  // All memory operations must set TR to 0, and all register operations must set TR to 1.
  // This is required to interpret the ID register correctly.
  // Set copies data from DP into the target address and the X variant performs a read-XOR-write with the data. The x
  // variant is very helpful for encoding traces, whereas the base version is more useful for register blasting.
  // Both are programmed the same way. While not mandatory, there is no convenient way to set ACCESS,ID,OFF registers.
  // Data always comes from DP/DS; see SETMEMI for the immediate form.
  // Packet registers: ACCESS, ID.lo, OFF.hi, OFF.lo
  // Successfully accesses must set F to 0. Failed acceses must set F to 1.
  SETMEM = 0x0010,
  SETMEMX = 0x0012,
  // Almost identical to mem variants, except that the ID register is 2 words rather than 1 and is not present.
  // Registers can't exceed 64-bits / DS==8. Sets F on memory access failure.
  // Data always comes from DP/DS; see SETREGI for the immediate form.
  // Packet registers: ACCESS, ID.hi, ID.lo
  SETREG = 0x0011,
  SETREGX = 0x0013,
  // Compare memory at DP with the target at offset, setting status bits accordingly
  // Data always comes from DP/DS; see CMPMEMI for the immediate form.
  // Packet registers: ID.lo, OFF.hi, OFF.lo
  // Same deal on F.
  CMPMEM = 0x0014,
  // Same as CMPMEM, except that the ID register is 2 words and there is no offset into register.
  // Data always comes from DP/DS; see CMPREGI for the immediate form.
  // If data size != register size, hard stops.
  // Packet registers:  ID.hi, ID.lo
  // Same deal on F.
  CMPREG = 0x0015,
  // Clear the memory module of a target
  // MOD1.lo contains the reset value, which will be masked to 1 byte. If not provided, assumed to be 0.
  // Packet registers: ID.lo,  MOD1.lo,
  // Same deal on F.
  CLRMEM = 0x0016,
  // Reset register to all 0.
  // Packet registers: ID.hi, ID.lo
  // Same deal on F.
  CLRREG = 0x0017,
  // Instructions which explicitly modify the way (source, address) is translated to (target, offset).
  // Those translations are actually used to create a /reverse/ map, which maps (target, offset) to a
  // (source,address).
  // Reverse address translation is a requirement to make updating the memory dump faster.
  // For memories whose address translation changes over time (e.g., caches), you will need to insert extra, custom
  // opcodes to update the translation table. This op is only really helpful for fixed translations, as it is not
  // invertible. Making invertible translations is a lot easier if you add new per-device opcodes, since they can
  // depend on the structure of that device rather than creating some insane, generic mechanism.
  // ID.hi holds source. ID.lo hold target. OFF.hi/lo holds target address. MOD2.hi/lo holds source address.
  // MOD1.hi/lo hold the translation in size.
  // Packet registers: (target) ID.lo, (target addr hi) OFF.hi, (target addr lo) OFF.lo, (source) ID.hi,
  //                   (source addr hi) MOD2.hi, (source addr lo) MOD2.lo, (size lo)MOD1.lo (size hi)MOD1.hi
  TRADDR = 0x0018,
  // Explicitly load DP and DS registers.
  // Packet registers: DP.lo, DS, DP.hi
  LDP = 0x0019,
  /*
   * This begins a section of DP-relative operations, which can be used to reduce code size for repeated operations if
   * you store data sequentially.
   */
  // "Accumulate" DS into DP.lo / aka add DS to DP.lo and load a new DS.
  // When data is tightly packed w/o alignment in data buffer, this gives us a 32-bit op to form a new data pointer.
  // Packet registers: DS
  ACCDP = 0x001A,
  // Variant of ACCDP where the DP.lo increment is explicitly provided. If your data is aligned, you are basically
  // required to use this variant.
  // Packet registers: increment for DP.lo, DS
  INCDP = 0x001B,

  // Conditional call back
  // MOD1.lo contains a condition code, MOD2.hi/lo contains the index of a callback function.
  // Callback is invoked in condition code DOES NOT match the current condition code. Useful for checking if register
  // assertions fail.
  CCB = 0x001C,
  // Return from an INVCALL, restoring the caller's replay direction. Distinct from RET so that unwinding an invertible
  // subroutine is explicit: RET is used by ordinary calls nested inside one, and must not end the suspension.
  // No packet registers.
  INVRET = 0x001D,
  // SETMEMX carries OFF in its packet, which makes the body of a program that stores to a different address each
  // time unique, preventing it from being promoted to a stencil. For SETMEMDX, the first 4 bytes at DP are the offset
  // (OFF.hi, OFF.lo) each. The DS and payload bytes follow as in SETMEM/X. With the unique portion (offset) moved out
  // of the instruction packet, we have more opportunities to promote instructions to stencils.
  //
  // When promoted to a stencil, SETMEMDX costs 4 bytes/instr more than SETMEM/X. For memory devices with predictable
  // access patterns across an entire instruction (register banks, CSRs), prefer SETMEM/X.
  //
  // DS remains the *payload* size, not the size of the whole DP carveout. That means DP-relative stepping cannot use
  // ACCDP after this, since ACCDP advances by DS and would land 4 bytes short. Use INCDP instead.
  //
  // Always XOR-encoded, because this is a specialized instruction to optimize invertible traces.
  // Packet registers: ACCESS, ID.lo
  SETMEMDX = 0x001E,
  // An operation that modifies a FIFORegister. FIFOs can change state on read, which makes them different than other
  // memory types. I've created a unified opcode for both reading and writing to FIFOs rather than modify SETMEM/D/X
  // which handles both reading and writing.
  // Sets DS to 1 for the sizeof data.
  //
  // Data pointer contains: (4)OFFSET, (1)DATA.
  // Packet registers: ACCESS, ID.lo, MOD1.lo = rd^wr
  MMIO = 0x001F,
  // STEP* are similar to SET*X in that they perform a read-modify-write of a memory location. The difference is that
  // the modification operation is signed addition rather than XOR. This is useful for (program) counters.
  // Data always comes from DP/DS; see STEPMEMI for the immediate form.
  // If MOD1.hi is 0, then the target location should be interpreted as a LE number; if 1, BE.
  // read-modify-written. Packet registers should remain LE.
  //
  // Packet registers: ACCESS, ID.lo, OFF.hi, OFF.lo, MOD1.hi
  STEPMEM = 0x0020,
  // Packet registers: ACCESS, ID.hi, ID.lo
  STEPREG = 0x0022,
  // A non-invertible copy from memory to register.
  // ACCESS is only used for the read, register write occurs with Host permissions. ID hold the destination (therefore
  // TR=1). OFF hold the source memory address. DS is inferred from the register's size. MOD1.lo contains the ID of the
  // source memory device. If MOD1.hi is 1, then the value should be byteswapped before being written to the
  // destination.
  //
  // Packet registers: ACCESS, ID.hi, ID.lo,OFF.hi, OFF.lo, MOD1.hi, MOD1.lo
  MOVMREG = 0x0023,
  // A non-invertible copy from a ELF segment to a Loadable device.
  // ID.lo contains the loadable destination device's ID. ACCESS is one of the enumerated values of MemoryKind rather
  // than our typical read/write/execute. Mod1.hi contains the file index, and Mod1.lo contains the segment index.
  // Memory offsets & sizes are derived from the segment in the backend.
  //
  // Packet registers: ACCESS, ID.lo, MOD1.hi, MOD1.lo
  LDSEGM = 0x0024,
  // Load masked register.
  // First word is a bitmask which indicates which registers to load.
  // The mask is defined in RegMask.
  // We iterate bits right to left (0..14). If the bit is set, we read the next word into the target register.
  // The process continues until we run out of bits or we run out of words.
  // Originally, I had a different load instruction per register. This wasted a ton of opcode space & program encoding
  // space when setting more than one register at once. Take advantage of the varadicity man.
  // Packet registers: RegMask, <varies>
  LMR = 0x4000,
  // Immediate forms of SET*, STEP*, CMP* and *SYN, which decode to the same operations as their DP-relative
  // counterparts. The payload travels in the packet instead of at DP, leaving DP and DS untouched. The packet is the
  // base opcode's full packet, followed by a size word in MOD1.lo, followed by the payload bytes. MOD2 is set to point
  // at the payload.
  // Packet registers: <base packet>, MOD1.lo, <payload>
  SETMEMI = 0x4100,
  SETREGI = 0x4200,
  SETMEMXI = 0x4300,
  SETREGXI = 0x4400,
  STEPMEMI = 0x4500,
  STEPREGI = 0x4600,
  CMPMEMI = 0x4700,
  CMPREGI = 0x4800,
  // The sync ops have no base packet, so the size word comes first. MOD1.lo is clipped to 8 bytes.
  ASYNI = 0x4900,
  ISYNI = 0x4A00,
};

constexpr bool is_variable(Opcode op) { return ((u16)op & OPPLANE_MASK) == OPPLANE_VARIABLE; }

// Packet length in words of a fixed opcode, or -1 for a variable or unassigned one.
constexpr int fixed_words(Opcode op) {
  switch (op) {
  case Opcode::HALT: [[fallthrough]];
  case Opcode::RET: [[fallthrough]];
  case Opcode::INVRET: [[fallthrough]];
  case Opcode::ASYN: [[fallthrough]];
  case Opcode::ISYN: return 0;
  case Opcode::HALTC: [[fallthrough]];
  case Opcode::CALLN: [[fallthrough]];
  case Opcode::STCALL: [[fallthrough]];
  case Opcode::STCALLHALT: [[fallthrough]];
  case Opcode::BRFN: [[fallthrough]];
  case Opcode::NOPN: [[fallthrough]];
  case Opcode::BREQN: [[fallthrough]];
  case Opcode::BRGTN: [[fallthrough]];
  case Opcode::BRGEN: [[fallthrough]];
  case Opcode::BRLTN: [[fallthrough]];
  case Opcode::BRLEN: [[fallthrough]];
  case Opcode::BRNEN: [[fallthrough]];
  case Opcode::BRN: [[fallthrough]];
  case Opcode::ACCDP: return 1;
  case Opcode::CALL: [[fallthrough]];
  case Opcode::CALLHALT: [[fallthrough]];
  case Opcode::INVCALLN: [[fallthrough]];
  case Opcode::BRF: [[fallthrough]];
  case Opcode::NOP: [[fallthrough]];
  case Opcode::BREQ: [[fallthrough]];
  case Opcode::BRGT: [[fallthrough]];
  case Opcode::BRGE: [[fallthrough]];
  case Opcode::BRLT: [[fallthrough]];
  case Opcode::BRLE: [[fallthrough]];
  case Opcode::BRNE: [[fallthrough]];
  case Opcode::BR: [[fallthrough]];
  case Opcode::SETMEMDX: [[fallthrough]];
  case Opcode::CMPREG: [[fallthrough]];
  case Opcode::CLRMEM: [[fallthrough]];
  case Opcode::CLRREG: [[fallthrough]];
  case Opcode::INCDP: return 2;
  case Opcode::SETREG: [[fallthrough]];
  case Opcode::SETREGX: [[fallthrough]];
  case Opcode::STEPREG: [[fallthrough]];
  case Opcode::CMPMEM: [[fallthrough]];
  case Opcode::LDP: [[fallthrough]];
  case Opcode::MMIO: return 3;
  case Opcode::SETMEM: [[fallthrough]];
  case Opcode::SETMEMX: [[fallthrough]];
  case Opcode::INVCALL: [[fallthrough]];
  case Opcode::LDSEGM: return 4;
  case Opcode::STEPMEM: return 5;
  case Opcode::MOVMREG: return 7;
  case Opcode::TRADDR: return 8;
  default: return -1;
  }
}

// (4) OFFSET
inline constexpr u16 SETMEMDX_ADDRESS_BYTES = 4;
// (4) OFFSET
inline constexpr u16 MMIO_PROLOGUE_BYTES = 4;

// Instructions to the tvm::Interpreter are always multiples of 16bits
struct OpWord {
  static constexpr u16 CLRMOD_BIT = 0x8000;
  constexpr OpWord() = default;
  // Crack a raw instruction word.
  constexpr explicit OpWord(u16 word) : clrmod((word & CLRMOD_BIT) != 0) {
    const u16 raw = word & OPCODE_MASK;
    if ((raw & OPPLANE_MASK) == OPPLANE_VARIABLE) {
      opcode = static_cast<Opcode>(raw & VARIABLE_OPCODE_MASK);
      word_len = static_cast<u8>(raw & 0xFF);
    } else {
      opcode = static_cast<Opcode>(raw);
      const int fixed = fixed_words(opcode);
      word_len = fixed < 0 ? 0 : static_cast<u8>(fixed);
    }
  }
  // word_len is only encoded for variable opcodes; a fixed opcode's comes from fixed_words().
  constexpr OpWord(Opcode op, bool clrmod, u8 word_len) : opcode(op), clrmod(clrmod), word_len(word_len) {}
  constexpr OpWord(const OpWord &) = default;
  constexpr OpWord &operator=(const OpWord &) = default;
  constexpr OpWord(OpWord &&) = default;
  constexpr OpWord &operator=(OpWord &&) = default;
  Opcode opcode = Opcode::HALT;
  // If 1, reset both MOD1 and MOD2 registers after executing this instruction
  bool clrmod = false;
  // Number of 16-bit words in this instruction packet, not including this 16-bit word.
  // Assuming the opcode is NOT a branch, final IP.lo will be incremented by 2 + word_len*2.
  u8 word_len = 0;
  constexpr u16 as_u16() const {
    const u16 len = is_variable(opcode) ? word_len : 0;
    return (clrmod ? CLRMOD_BIT : 0) | static_cast<u16>(opcode) | len;
  }
};

// Modifications to tvm::Interpreter::Sate also require updating these register masks.
enum class RegMask : u16 {
  IP_HI = 1 << 0,
  IP_LO = 1 << 1,
  DP_HI = 1 << 2,
  DP_LO = 1 << 3,
  DS = 1 << 4,
  ACCESS = 1 << 5,
  ID_HI = 1 << 6,
  ID_LO = 1 << 7,
  OFF_HI = 1 << 8,
  OFF_LO = 1 << 9,
  MOD1_HI = 1 << 10,
  MOD1_LO = 1 << 11,
  MOD2_HI = 1 << 12,
  MOD2_LO = 1 << 13,
  // Not really a register, but make it act like one.
  // Must mask writes to L bit because otherwise it's too easy to halt the blaster.
  FLAGS = 1 << 14,
  ALL = (FLAGS << 1) - 1,
};

// Our branches compute the (E)quals, (G)reater than, (L)ess than, and (F)ail conditions, which are *'ed together the
// condition code. If any bit is set in the result, the branch is taken. This lets us synthesize every conditional
// branch type.
enum class ConditionCode : u16 {
  E = 1 << 0, //
  G = 1 << 1,
  L = 1 << 2,
  F = 1 << 3,
  MASK = ((u8)F << 1) - 1,
};

// Represent a u32 as a pair of u16 and handle packing automatically.
struct SegmentPair {
  u16 hi = 0;
  u16 lo = 0;
  u32 as_u32() const { return (static_cast<u32>(hi) << 16) | static_cast<u32>(lo); }
};

} // namespace tvm

consteval void is_bitflags(tvm::RegMask);
consteval void is_bitflags(tvm::ConditionCode);