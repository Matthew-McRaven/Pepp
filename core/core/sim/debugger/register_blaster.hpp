#pragma once
#include <array>
#include <functional>
#include <memory>
#include "core/ds/alloc/pagechain.hpp"
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
  // Push next IP onto SP. Set IP.lo to MOD1.lo and IP.hi to MOD1.hi.
  // 2 Packet registers: IP.lo<=MOD1.lo, ip.hi<=MOD1.hi
  CALL = 0b00'0010,
  // A synchronization opcode, used to communicate a clock tick to the blaster.
  // MOD1 contains the lo order of the timestamp as (hi, lo) and MOD2 contains the hi order of the timestamp as (hi,
  // lo).
  // 4 Packet registers: MOD1.hi, MOD1.lo, MOD2.hi, MOD2.lo
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
  // Packet registers: ACCESS, ID.lo, OFF.hi, OFF.lo
  // Successfully accesses must set F to 0. Failed acceses must set F to 1.
  SETMEM = 0b01'0000,
  SETMEMX = 0b01'0010,
  // Almost identical to mem variants, except that the ID register is 2 words rather than 1 and offset is a single
  // word. Registers can't exceed 64-bits / DS==8. Sets F on failure.
  // Packet registers: ACCESS, ID.hi, ID.lo, OFF.lo.
  SETREG = 0b01'0001,
  SETREGX = 0b01'0011,
  // Compare memory at DP with the target at offset.
  // MOD1 is assumed to hold a condition code (lge) and MOD2 holds a comparison callback address. If the comparison
  // does not match condition code, the callback is invoked.
  // Packet registers: MOD1.LO, ID.lo, OFF.hi, OFF.lo
  // Same deal on F.
  CMPMEM = 0b01'0100,
  // Same as CMPMEM, except that the ID register is 2 words and there is no offset into register.
  // If DS != register size, hard stops.
  // Packet registers: MOD1.lo, ID.hi, ID.lo
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
  // ID.hi holds source. ID.lo hold target. OFF.hi/lo holds target address. DP.hi/lo holds source address.
  // DS hold the translation in size.
  // Packet registers: (target) ID.lo, (target addr hi) OFF.hi, (target addr lo) OFF.lo, (source) ID.hi,
  //                   (source addr hi) DP.hi, (source addr lo) DP.lo, (size)DS
  TRADDR = 0b01'1000,
  // Packet containing inline data.
  // DS is the first word and is required. DP is set to the address of the following word.
  // All remaining words are treated as data.
  // Packet registers: DS
  LDPI = 0b01'1001,
  // Load DP and DS registers.
  // Packet registers: DP.lo, DS, DP.hi
  LDP = 0b01'1010,
  // Must always be 1 greater than the last opcode. Used to size the decoder table at compile-time.
  MAX = ((u8)LDP) + 1,
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
  u16 as_u16() const {
    const u8 op_byte = static_cast<u8>((clrmod ? 1 : 0) << CLRSHIFT) | static_cast<u8>(ocpode);
    return (static_cast<u16>(op_byte) << 8) | static_cast<u16>(word_len);
  }
};

// Represent a u32 as a pair of u16 and handle packing automatically.
struct SegmentPair {
  u16 hi = 0;
  u16 lo = 0;
  u32 as_u32() const { return (static_cast<u32>(hi) << 16) | static_cast<u32>(lo); }
};

namespace EncodedOp {
using StopCause = StopCause;
using SegmentPair = SegmentPair;

template <Opcode Op, bool clrmod, typename... M> constexpr std::array<u8, 2 * (1 + sizeof...(M))> encode_op(M... mods) {
  static_assert((std::is_convertible_v<M, u16> && ...), "mod words must be u16");
  const std::array<u16, 1 + sizeof...(M)> words = {OpWord(Op, clrmod, sizeof...(M)).as_u16(),
                                                   static_cast<u16>(mods)...};
  std::array<u8, 2 * (1 + sizeof...(M))> bytes{};
  // Virtual machine is little endian because that matches most common host archs.
  for (std::size_t i = 0; i < words.size(); ++i) {
    bytes[2 * i] = static_cast<u8>(words[i] & 0xFF);
    bytes[2 * i + 1] = static_cast<u8>((words[i] >> 8) & 0xFF);
  }
  return bytes;
}

struct Halt_1 {
  StopCause cause;
  constexpr auto encode() const { return encode_op<Opcode::HALT, true>(static_cast<u16>(cause)); };
};
struct Halt_0 {
  constexpr auto encode() const { return encode_op<Opcode::HALT, true>(); };
};

struct Ret_0 {
  constexpr auto encode() const { return encode_op<Opcode::RET, true>(); }
};

struct Call_2 {
  SegmentPair next_ip;
  constexpr auto encode() const { return encode_op<Opcode::CALL, true>(next_ip.lo, next_ip.hi); }
};
struct Call_1 {
  u16 next_ip_lo;
  constexpr auto encode() const { return encode_op<Opcode::CALL, true>(next_ip_lo); }
};
struct Call_0 {
  constexpr auto encode() const { return encode_op<Opcode::CALL, true>(); }
};
struct Syn_4 {
  SegmentPair timestamp_lo;
  SegmentPair timestamp_hi;
  constexpr auto encode() const {
    return encode_op<Opcode::SYN, true>(timestamp_lo.hi, timestamp_lo.lo, timestamp_hi.hi, timestamp_hi.lo);
  }
};
struct Syn_2 {
  SegmentPair timestamp_lo;
  constexpr auto encode() const { return encode_op<Opcode::SYN, true>(timestamp_lo.hi, timestamp_lo.lo); }
};

struct Syn_0 {
  SegmentPair timestamp_lo;
  constexpr auto encode() const { return encode_op<Opcode::SYN, true>(); }
};

// Use lmr/lmr_of if you want to emit LMR instructions. They're variadic. I won't help you with a struct because that
// struct will be way too fat or it will incur dynamic memory alloc.
template <bool clrmod, std::size_t N> constexpr auto LMR(std::array<std::pair<RegMask, u16>, N> pairs) {
  // Sort by mask value ascending (smallest bit first) — insertion sort, constexpr-friendly.
  for (std::size_t i = 1; i < N; ++i) {
    auto key = pairs[i];
    std::size_t j = i;
    while (j > 0 && static_cast<u16>(pairs[j - 1].first) > static_cast<u16>(key.first)) {
      pairs[j] = pairs[j - 1];
      --j;
    }
    pairs[j] = key;
  }

  // OR all masks into the combined mask word.
  u16 combined = 0;
  for (auto &p : pairs) combined |= static_cast<u16>(p.first);

  // Pull sorted values into an index sequence so we can expand into emit().
  return [&]<std::size_t... I>(std::index_sequence<I...>) {
    return encode_op<Opcode::LMR, clrmod>(combined, pairs[I].second...);
  }(std::make_index_sequence<N>{});
}
template <bool clrmod = true, typename... P> constexpr auto LMR_of(P... pairs) {
  using RM = RegMask;
  return LMR<clrmod>(std::array<std::pair<RM, u16>, sizeof...(P)>{pairs...});
}

struct LDMOD1Hi_1 {
  u16 value;
  constexpr auto encode() const { return encode_op<Opcode::LMR, false>((u16)RegMask::MOD1_HI, value); }
};

struct LDMOD1Lo_1 {
  u16 value;
  constexpr auto encode() const { return encode_op<Opcode::LMR, false>((u16)RegMask::MOD1_LO, value); }
};

struct LDMOD2Hi_1 {
  u16 value;
  constexpr auto encode() const { return encode_op<Opcode::LMR, false>((u16)RegMask::MOD2_HI, value); }
};

struct LDMOD2Lo_1 {
  u16 value;
  constexpr auto encode() const { return encode_op<Opcode::LMR, false>((u16)RegMask::MOD2_LO, value); }
};

template <Opcode BRT> struct _BR_2 {
  SegmentPair displacement;
  constexpr auto encode() const { return encode_op<BRT, true>(displacement.lo, displacement.hi); }
};
template <Opcode BRT> struct _BR_1 {
  u16 displacement_lo;
  constexpr auto encode() const { return encode_op<BRT, true>(displacement_lo); }
};
template <Opcode BRT> struct _BR_0 {
  constexpr auto encode() const { return encode_op<BRT, true>(); }
};

// Create 2/1/0 variants for NOP / BREQ / BRGT / BRGE / BRLT / BRLE / BRNE / BR using the above templates

using NOP_0 = _BR_0<Opcode::NOP>;
using BREQ_2 = _BR_2<Opcode::BREQ>;
using BREQ_1 = _BR_1<Opcode::BREQ>;
using BRGT_2 = _BR_2<Opcode::BRGT>;
using BRGT_1 = _BR_1<Opcode::BRGT>;
using BRGE_2 = _BR_2<Opcode::BRGE>;
using BRGE_1 = _BR_1<Opcode::BRGE>;
using BRLT_2 = _BR_2<Opcode::BRLT>;
using BRLT_1 = _BR_1<Opcode::BRLT>;
using BRLE_2 = _BR_2<Opcode::BRLE>;
using BRLE_1 = _BR_1<Opcode::BRLE>;
using BRNE_2 = _BR_2<Opcode::BRNE>;
using BRNE_1 = _BR_1<Opcode::BRNE>;
using BR_2 = _BR_2<Opcode::BR>;
using BR_1 = _BR_1<Opcode::BR>;

// LDPI is variadic width b/c of the way we load data.
// So, give me bytes and I'll encode a packet for you and set DS automatically.

// Compile-time packing of bytes into LE words for use by encode_op.
template <std::size_t N> constexpr auto pack_bytes(std::array<u8, N> data) {
  constexpr std::size_t WordCount = (N + 1) / 2;
  std::array<u16, WordCount> words{};
  for (std::size_t i = 0; i < N; i += 2) {
    u16 w = data[i];
    if (i + 1 < N) w |= static_cast<u16>(data[i + 1]) << 8;
    words[i / 2] = w;
  }
  return words;
}

template <std::size_t N> constexpr auto ldpi(std::array<u8, N> data) {
  auto words = pack_bytes(data);
  return encode_op<Opcode::LDPI, true>((u16)N, words);
}

template <std::size_t N> constexpr auto ldpi(std::array<u16, N> words) {
  return encode_op<Opcode::LDPI, true>((u16)N * 2, words);
}

template <typename... W> constexpr auto ldpi_w(W... ws) {
  static_assert((std::is_convertible_v<W, u16> && ...), "words must be u16");
  constexpr std::size_t N = sizeof...(W);
  return encode_op<Opcode::LDPI, true>(2 * (u16)N, ws...);
}

struct LDP_3 {
  SegmentPair DP;
  u16 DS;
  constexpr auto encode() const { return encode_op<Opcode::LDP, true>(DP.lo, DS, DP.hi); }
};
struct LDP_2 {
  SegmentPair DP;
  constexpr auto encode() const { return encode_op<Opcode::LDP, true>(DP.lo, DP.hi); }
};
struct LDP_1 {
  u16 DP_lo;
  constexpr auto encode() const { return encode_op<Opcode::LDP, true>(DP_lo); }
};

} // namespace EncodedOp
} // namespace tvm

// This is basically an ASIC with a custom instruction set used to copy  values into a simulator's
// registers+memory.
// The machine uses little-endian 16-bit words. Opcodes are always one word and called OpWords. Instructions are a
// single 16-bit opcode followed a per-opcode number of 16-bit data words.
// I borrow the concept of a register programming engine that is bytecode-programmable from [AMD's
// Atombios](https://wiki.osdev.org/AMD_Atombios) and the Nova-Core GSP sequencer. Based on an opcode, the blaster
// chooses how to reprogram registers in the target or update internal state.
// Since this blaster will be used for both initial register programming AND tracing, I have extreme pressure to reduce
// memory footprint, even if it introduces dependencies between instructions. The Xilinx 7 series bitstream format
// carries dependencies between its type-1 vs type-2 configuration packets. type-1 packets set initial state (and
// address) which is reused by future type-2 packets. That is, register state is /retained/. Another analogue is JTAG's
// TAP's IR register being retained across data scan registers operations.
// So, what if I retain data in my registers across operations? Well, I'd want to ditch the fixed-sized opcodes of
// Atombios and Nova-Core, since those have decoder-determined instruction lengths. In my design, the length (in u16s)
// is explicitly encoded in the opcode rather than being implicit in the decoder. With a 16-bit opcode I have plenty of
// bits to spare. Because of the variadicity, I reduce the memory footprint for best cases:
// - When an instruction contains less than the number of expected registers, the remaining  registers are retained.
//   With intelligent design-time ordering of operands, common fields can be reused between instructions.
//   For example, short branches can be encoded with a single word rather than 2.
// - When more words are inserted than expected, extra values are ignored.
//   This case is really useful for packing data into an instruction, and is explictly used by SET* instructions.
// In an average case, this reduces exactly to the same number of words as would be used by Atombios or Nova-core,
// except with some extra bookkeeping for the populated word count. My contributions over those previous designs
// include:
// - my XOR-encoding for SET*X instructions derives from my previous trace buffer encodings to make a single trace which
//   contains both the "forward" and "backward" data.
// - Explicit call+ret. My work on Pep9Micro taught me that subroutines simplify everything, and I should bake them in
//   from the start.
// - My 3-bit branch encoding, which allows synthesis of all branch types, including a NOP. It  might be the first ISA
//   in history to require a branch prediction for a NOP.
// - Optional registers (MOD1/MOD2) which provide optional data to an instruction that are cleared automatically.
//   Those registers really model a bag-of-properties like my old AST design. The MODCLR bit of an opcode helps clear
//   these automatically.
// While opcode decoding and register programming is handled by this class, the "implementation" of each opcode is
// customizable by providing a callback per-opcode.
// We provide a helper to install the same handler for all BR mnemonics for your convenience.

class RegisterBlaster {
public:
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

  struct State {
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
    // Neither of the following can be accessed via regmask
    // Stack pointer, used to make call/ret work.
    u16 SP = 0;
    // 16-bit integer which contains a decoded instruction.
    tvm::OpWord IS{};
  };

  // Yes, this pays an indirect call with some trampoline magic, but it allows the register blaster and trace buffer to
  // become the same class. That execution speed penalty is more than worth it to me to consolidate the two types.
  using CMPCallback = std::function<void(RegisterBlaster &, bool)>;
  // Non-owning pointer to system.
  RegisterBlaster(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system = nullptr);
  // Disable copy/move since this class is EXPENSIVE
  RegisterBlaster(const RegisterBlaster &) = delete;
  RegisterBlaster(RegisterBlaster &&) = delete;
  RegisterBlaster &operator=(const RegisterBlaster &) = delete;
  RegisterBlaster &operator=(RegisterBlaster &&) = delete;

  void update_ip(pepp::bts::Buffer::Location loc);
  void update_ip(pepp::bts::Buffer::ID, u16 offset = 0);
  // Assuming some code is already under IP, try to run it!
  void step();
  // Update IP to point to loc, then call step() in a loop while L==1.
  // Each program executed this way must terminate with a HALT.
  // At the end of a call to run_direct, L is always 0.
  void run_direct(pepp::bts::Buffer::Location loc);
  // For each buffer location set L=1 and call run_direct.
  // Only stops when reacing the end of this buffer, or on "hard stop", where L==0 && F==1.
  void run_indirect(std::span<pepp::bts::Buffer::Location> locs);
  u16 register_cmp_callback(CMPCallback cb) {
    u16 id = _cmp_callbacks.size();
    _cmp_callbacks.push_back(cb);
    return id;
  }
  auto &csrs() { return _csrs; }
  const auto &csrs() const { return _csrs; }
  auto &regs() { return _regs; }
  const auto &regs() const { return _regs; }
  pepp::bts::BufferManager &mgr() { return *_mgr; }
  const pepp::bts::BufferManager &mgr() const { return *_mgr; }
  pepp::bts::Buffer *ibuffer();

protected:
  void soft_stop(tvm::StopCause cause = tvm::StopCause::None);
  void hard_stop(tvm::StopCause cause = tvm::StopCause::None);
  // Assuming register state is already set, execute the instruction. It is virtual so you can change execution behavior
  // in subclasses.
  virtual void execute();
  void execute_cmpreg();
  void execute_cmpmem();

private:
  // Fetch the word under IP, increment the IP, and set the registers & flags according to the decoded opcode.
  void decode();

  // Perform an LE read of 2 bytes at an offset.
  u16 read16(pepp::bts::Buffer::ID, u16 offset);
  // SP -= 4 and return the 4 bytes. IF SP would underflow stack, set L=0 and set cause in MOD1.lo
  tvm::SegmentPair pop();
  // SP +=4 and write the 4 bytes. If SP would overflow stack, set L = 0 and set cause in MOD1.lo
  void push(tvm::SegmentPair v);
  std::shared_ptr<pepp::bts::BufferManager> _mgr;
  System *_system = nullptr;
  RegisterScan *_scan;
  std::array<u8, 256> _stack;
  // Should really be a map so you can delete callbacks, but I can't be bothered to add the ID variable right now.
  std::vector<CMPCallback> _cmp_callbacks;
  Flags _csrs{};
  State _regs{};
  std::vector<u8> _tmp;
};

consteval void is_bitflags(tvm::RegMask);
consteval void is_bitflags(tvm::ConditionCode);

// Decoded Ops may include extra fields
namespace DecodedOp {
using SegmentPair = tvm::SegmentPair;
using Halt = tvm::EncodedOp::Halt_1;
struct Ret {
  SegmentPair next_ip;
};
struct Call {
  SegmentPair next_ip;
  SegmentPair ret_ip;
};
struct Syn {
  SegmentPair timestamp_lo;
  SegmentPair timestamp_hi;
};
struct LMR {
  tvm::RegMask mask;
  std::span<u16> regs;
};
struct BR {
  tvm::ConditionCode condition;
  SegmentPair displacement;
};
struct LDPI {
  std::span<u16> data;
};
struct LDP {
  SegmentPair DP;
  u16 DS;
};
} // namespace DecodedOp
// Helpers
template <typename... M> auto setmem(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::SETMEM, true>(m...); }
template <typename... M> auto setmemx(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::SETMEMX, true>(m...); }
template <typename... M> auto cmpmem(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::CMPMEM, true>(m...); }
template <typename... M> auto clrmem(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::CLRMEM, true>(m...); }
template <typename... M> auto setreg(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::SETREG, true>(m...); }
template <typename... M> auto setregx(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::SETREGX, true>(m...); }
template <typename... M> auto cmpreg(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::CMPREG, true>(m...); }
template <typename... M> auto clrreg(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::CLRREG, true>(m...); }
template <typename... M> auto traddr(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::TRADDR, true>(m...); }
