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
  // A synchronization opcode, used to communicate a clock tick to the blaster.
  // MOD2 contains the lo order of the timestamp as (hi, lo) and MOD1 contains the hi order of the timestamp as (hi,
  // lo).
  // 4 Packet registers: MOD2.hi, MOD2.lo, MOD1.hi, MOD1.lo
  SYN = 0b00'0011, // Unused opcode
  // Load masked register.
  // First word is a bitmask which indicates which registers to load.
  // The mask is defined in RegMask.
  // We iterate bits right to left (0..14). If the bit is set, we read the next word into the target register.
  // The process continues until we run out of bits or we run out of words.
  // Originally, I had a different load instruction per register. This wasted a ton of opcode space & program encoding
  // space when setting more than one register at once. Take advantage of the varadicity man.
  // Packet registers: RegMask, <varies>
  LMR = 0b00'0100,
  // GAP101 = 0b00'0101,
  // GAP110 = 0b00'0110,
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
  // MOD1.lo contains the reset value, which will be masked to 1 byte
  // Packet registers: MOD1.lo, ID.lo
  // Same deal on F.
  CLRMEM = 0b01'0110,
  // Reset register to default value as specified by the RegisterScan.
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
  // Must always be 1 greater than the last opcode. Used to size the decoder table at compile-time.
  MAX = ((u8)CCB) + 1,
};

// Instructions to the RegisterBlaster are always multiples of 16bits
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

// Modifications to RegisterBlaster::Sate also require updating these register masks.
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