#pragma once
#include "core/integers.h"

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
};

// Must fit into 6 bits because of the OpWord struct.
enum class Opcode : u8 {
  // Set L flag to 0, halting the blaster and F to 0. MOD1 is cause, MOD2 is ignored.
  // If the machine is halted and L==0 and F==1, it "hard stop". L==0 and F==0 is a "soft stop".
  // 1 Packet registers: MOD1.lo
  HALT = 0b00'0000,
  // Pop IP from SP. MOD1 and MOD2 are ignored
  RET = 0b00'0001,
  // Push next IP onto SP. Set IP.lo to MOD2.lo and IP.hi to MOD2.hi. If MOD2.hi is not set, use IP.hi.
  // 2 Packet registers: MOD2.lo, MOD2.hi
  CALL = 0b00'0010,
  // Load masked register.
  // First word is a bitmask which indicates which registers to load.
  // The mask is defined in RegMask.
  // We iterate bits right to left (0..14). If the bit is set, we read the next word into the target register.
  // The process continues until we run out of bits or we run out of words.
  // Originally, I had a different load instruction per register. This wasted a ton of opcode space & program encoding
  // space when setting more than one register at once. Take advantage of the varadicity man.
  // Packet registers: RegMask, <varies>
  LMR = 0b00'0011,
  // Synchronize absolute and synchronize incremental, which both take a timestamp / clock tick.
  // ASYN reports the full timestamp, whereas ISYN reports a signed delta to be added to the previous timestamp.
  // The two differ only in LSB, which is set for the incremental variant.
  // Both accept immediate or DP-relative data.
  // If MOD1.lo is set, is is treated as the size in bytes of the immeidate data, and MOD2 is set to point to the word
  // following MOD1.lo in the instruction stream. All remaining words in the packet are treated as data.
  // If MOD1.lo is not provided, data is located at DP.
  // This is the same immediate-vs-DP split used by SET*/CMP*.
  // The data is a little-endian integer. A timestamp can't exceed 64 bits, so the resulting size will be clipped to 8
  // bytes, regardless of data source. The blaster does not retain a timestamp, so this value is purely for higher-level
  // analysis code.
  // Packet registers: MOD1.lo
  ASYN = 0b00'0100,
  ISYN = 0b00'0101,
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
  // The two targets are interleaved lo-first so that the near case (both targets in the current buffer) fits in 2
  // words, the same trick the branches play with MOD2. Any target word that isn't supplied defaults to the
  // fall-through: a missing hi becomes IP.hi, and a wholly missing target becomes the next instruction.
  // MOD1 holds the forward target, MOD2 holds the backward target.
  // Packet registers: (forward).lo, (backward).lo, (forward).hi, (backward).hi
  INVCALL = 0b00'0110,
  // Branch if F bit is set, using the same packet registers are comparison branches.
  // Sets MOD1.lo to ConditionCode::F.
  // 2 Packet registers: MOD2.lo, MOD2.hi
  BRF = 0b00'0111,
  // All branch instructions, with bit pattern  01 0lge. It's a bit of a psychotic encoding, allowing you to select
  // between 3 conditions simultaneously: (l) less than, (g) greater than, and (e) equal. This bits are sufficient to
  // synthesize all meaningful branch conditions, plus an uncondtional branch and noop.
  // Condition is encoded in opcode bits rather than a packet word to save space.
  // MOD1 will always be set to the condition code bits (bge). MOD2 is the displacement added to the IP if the branch
  // is taken. Reverse typical hi/lo to reduce # of bytes required for near branch.
  // 2 Packet registers: MOD2.lo, MOD2.hi
  // If MOD2.hi is included, then MOD2.hi is automatically set to IP.hi
  NOP = 0b00'1000,
  BREQ = 0b00'1001,
  BRGT = 0b00'1010,
  BRGE = 0b00'1011,
  BRLT = 0b00'1100,
  BRLE = 0b00'1101,
  BRNE = 0b00'1110,
  BR = 0b00'1111,
  // From here we begin memory / register operations. Memory operation act directly on a Target*, whereas Register
  // operations operate on a RegisterRef from a RegisterScan. While the encoding bits interleave mem/reg ops, they are
  // enumerated separately because they have different semantics.
  // All memory operations must set TR to 0, and all register operations must set TR to 1.
  // This is required to interpret the ID register correctly.
  // Set copies data from DP into the target address and the X variant performs a read-XOR-write with the data. The x
  // variant is very helpful for encoding traces, whereas the base version is more useful for register blasting.
  // Both are programmed the same way. While not mandatory, there is no convenient way to set ACCESS,ID,OFF registers.
  // If MOD1 is provided, it is used as a temporary override for size. In this case MOD2.hi is set to IP.hi,
  // and MOD2.lo is set to the location 6. Passing IP-relative data via MOD1/MOD2 is the "immediate" variant.
  // Packet registers: ACCESS, ID.lo, OFF.hi, OFF.lo, MOD1.lo
  // Successfully accesses must set F to 0. Failed acceses must set F to 1.
  SETMEM = 0b01'0000,
  SETMEMX = 0b01'0010,
  // Almost identical to mem variants, except that the ID register is 2 words rather than 1 and is not present.
  // Registers can't exceed 64-bits / DS==8. Sets F on memory access failure.
  // If MOD1 is provided, it is used as a temporary override for size. In this case MOD2.hi is set to IP.hi,
  // and MOD2.lo is set to the location 5. Passing IP-relative data via MOD1/MOD2 is the "immediate" variant.
  // Packet registers: ACCESS, ID.hi, ID.lo, MOD1.lo
  SETREG = 0b01'0001,
  SETREGX = 0b01'0011,
  // Compare memory at DP with the target at offset, setting status bits accordingly
  // If MOD1.lo is provided, it uses the same immediate data semantics as SETMEM.
  // Packet registers: ID.lo, OFF.hi, OFF.lo, MOD1.lo
  // Same deal on F.
  CMPMEM = 0b01'0100,
  // Same as CMPMEM, except that the ID register is 2 words and there is no offset into register.
  // If MOD1.lo is provided, it uses the same immediate data semantics as SETMEM.
  // If data size != register size, hard stops.
  // Packet registers:  ID.hi, ID.lo, MOD1.lo
  // Same deal on F.
  CMPREG = 0b01'0101,
  // Clear the memory module of a target
  // MOD1.lo contains the reset value, which will be masked to 1 byte. If not provided, assumed to be 0.
  // Packet registers: ID.lo,  MOD1.lo,
  // Same deal on F.
  CLRMEM = 0b01'0110,
  // Reset register to all 0.
  // Packet registers: ID.hi, ID.lo
  // Same deal on F.
  CLRREG = 0b01'0111,
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
  TRADDR = 0b01'1000,
  // Explicitly load DP and DS registers.
  // Packet registers: DP.lo, DS, DP.hi
  LDP = 0b01'1001,
  /*
   * This begins a section of DP-relative operations, which can be used to reduce code size for repeated operations if
   * you store data sequentially.
   */
  // "Accumulate" DS into DP.lo / aka add DS to DP.lo and load a new DS.
  // When data is tightly packed w/o alignment in data buffer, this gives us a 32-bit op to form a new data pointer.
  // Packet registers: DS
  ACCDP = 0b01'1010,
  // Variant of ACCDP where the DP.lo increment is explicitly provided. If your data is aligned, you are basically
  // required to use this variant.
  // Packet registers: increment for DP.lo, DS
  INCDP = 0b01'1011,

  // Conditional call back
  // MOD1.lo contains a condition code, MOD2.hi/lo contains the index of a callback function.
  // Callback is invoked in condition code DOES NOT match the current condition code. Useful for checking if register
  // assertions fail.
  CCB = 0b01'1100,
  // Return from an INVCALL, restoring the caller's replay direction. Distinct from RET so that unwinding an invertible
  // subroutine is explicit: RET is used by ordinary calls nested inside one, and must not end the suspension.
  // No packet registers.
  INVRET = 0b01'1101,
  // SETMEMX carries OFF in its packet, which makes the body of a program that stores to a different address each
  // time unique, preventing it from being templatized. For SETMEMDX,  the first 4 bytes at DP are the offset (OFF.hi,
  // OFF.lo) each. The DS and payload bytes follow as in SETMEM/X. With the unique portion (offset) move out of the
  // instruction packet, we have more opportunities to promote instructions to templates.
  //
  // When promoted to a template, SETMEMDX costs 4 bytes/instr more than SETMEM/X. For memory devices with predictable
  // access patterns across an entire instruction (register banks, CSRs), prefer SETMEM/X.
  //
  // DS remains the *payload* size, not the size of the whole DP carveout. That means DP-relative stepping cannot use
  // ACCDP after this, since ACCDP advances by DS and would land 4 bytes short. Use INCDP instead.
  //
  // Always XOR-encoded, because this is a specialized instruction to optimize invertible traces.
  // Packet registers: ACCESS, ID.lo
  SETMEMDX = 0b01'1110,
  // An operation that modifies a FIFORegister. FIFOs can change state on read, which makes them different than other
  // memory types. I've created a unified opcode for both reading and writing to FIFOs rather than modify SETMEM/D/X
  // which handles both reading and writing.
  // Sets DS to 1 for the sizeof data.
  //
  // Data pointer contains: (4)OFFSET, (1)DATA.
  // Packet registers: ACCESS, ID.lo, MOD1.lo = rd^wr
  MMIO = 0b01'1111,
  // Must always be 1 greater than the last opcode. Used to size the decoder table at compile-time.
  MAX = ((u8)MMIO) + 1,
};

// (4) OFFSET
inline constexpr u16 SETMEMDX_ADDRESS_BYTES = 4;
// (4) OFFSET
inline constexpr u16 MMIO_PROLOGUE_BYTES = 4;

// Instructions to the tvm::Interpreter are always multiples of 16bits
struct OpWord {
  static constexpr auto CLRSHIFT = 6;
  constexpr OpWord() = default;
  constexpr OpWord(u8 op_byte, u8 size) : word_len(size) {
    static constexpr auto OPMASK = (1 << CLRSHIFT) - 1;
    static constexpr auto CLRMODMASK = 1 << CLRSHIFT;
    ocpode = static_cast<u8>(op_byte & OPMASK);
    clrmod = (op_byte & CLRMODMASK) != 0;
  }
  // Delegate to the individual byte variant.
  constexpr OpWord(u16 word) : OpWord(word >> 8, word & 0xFF) {}
  constexpr OpWord(Opcode op, bool clrmod, u8 word_len)
      : ocpode(static_cast<u8>(op)), clrmod(clrmod), pad(0), word_len(word_len) {}
  constexpr OpWord(const OpWord &) = default;
  constexpr OpWord &operator=(const OpWord &) = default;
  constexpr OpWord(OpWord &&) = default;
  constexpr OpWord &operator=(OpWord &&) = default;
  // Which opcode does this instruction
  u16 ocpode : 6 = static_cast<u8>(Opcode::HALT);
  // If 1, reset both MOD1 and MOD2 registers after executing this instruction
  u16 clrmod : 1 = 0;
  // Unused
  u16 pad : 1 = 0;
  // Number of 16-bit words in this instruction packet, not including this 16-bit word.
  // Assuming the opcode is NOT a branch, final IP.lo will be incremented by 2 + word_len*2.
  u16 word_len : 8 = 0;
  constexpr u16 as_u16() const {
    const u8 op_byte = static_cast<u8>((clrmod ? 1 : 0) << CLRSHIFT) | static_cast<u8>(ocpode);
    return (static_cast<u16>(op_byte) << 8) | static_cast<u16>(word_len);
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