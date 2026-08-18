#pragma once
#include <span>
#include <variant>
#include "core/sim/api/device.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/debugger/tvm_opcodes.hpp"

namespace tvm {

// Helpers to encode a (opcode,size, data) into the expected little-endian byte stream.
// Useful for generating programs for the TVM.
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

// Helpers to show immediate data into opcode stream.
// Writes out size in bytes before the data bytes.
// This matches the typical "immediate" pattern, which loads a size into MOD1.lo and then treats all other bytes as
// payload.
template <Opcode Op, typename Derived> struct ImmediateEncoder {
  template <std::size_t N> constexpr auto encode(const std::array<u16, N> &data) const {
    return static_cast<const Derived &>(*this).apply_prefix([&](auto... prefix) {
      return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return encode_op<Op, true>(prefix..., 2 * (u16)N, data[I]...);
      }(std::make_index_sequence<N>{});
    });
  }
  template <std::size_t N> constexpr auto encode(std::array<u8, N> data) const {
    auto words = pack_bytes(data);
    // Odd counts must be rounded up to the next word.
    constexpr std::size_t WordCount = (N + 1) / 2;
    return static_cast<const Derived &>(*this).apply_prefix([&](auto... prefix) {
      return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return encode_op<Op, true>(prefix..., (u16)N, words[I]...);
      }(std::make_index_sequence<WordCount>{});
    });
  }
  template <typename... D>
    requires(std::is_convertible_v<D, u16> && ...)
  constexpr auto encode(D... data) const {
    return static_cast<const Derived &>(*this).apply_prefix(
        [&](auto... prefix) { return encode_op<Op, true>(prefix..., 2 * (u16)sizeof...(D), (u16)data...); });
  }
};

template <std::size_t> struct Halt;
template <> struct Halt<0> {
  constexpr auto encode() const { return encode_op<Opcode::HALT, true>(); };
};
template <> struct Halt<1> {
  StopCause cause;
  constexpr auto encode() const { return encode_op<Opcode::HALT, true>(static_cast<u16>(cause)); };
};

template <std::size_t> struct Ret;
template <> struct Ret<0> {
  constexpr auto encode() const { return encode_op<Opcode::RET, true>(); };
};

// Return from an invertible subroutine, restoring the caller's direction. See Opcode::INVRET.
template <std::size_t> struct InvRet;
template <> struct InvRet<0> {
  constexpr auto encode() const { return encode_op<Opcode::INVRET, true>(); };
};

template <std::size_t> struct Call;
template <> struct Call<0> {
  constexpr auto encode() const { return encode_op<Opcode::CALL, true>(); }
};
template <> struct Call<1> {
  u16 next_ip_lo;
  constexpr auto encode() const { return encode_op<Opcode::CALL, true>(next_ip_lo); }
};
template <> struct Call<2> {
  SegmentPair next_ip;
  constexpr auto encode() const { return encode_op<Opcode::CALL, true>(next_ip.lo, next_ip.hi); }
};

// Targets are interleaved lo-first, so the 2-word form reaches both targets within the current buffer.
// on_forward is called when stepping forward, on_backward when stepping backward. Anything not supplied falls through
// to the next instruction.
template <std::size_t> struct InvCall;
template <> struct InvCall<0> {
  constexpr auto encode() const { return encode_op<Opcode::INVCALL, true>(); }
};
template <> struct InvCall<1> {
  u16 on_forward_lo;
  constexpr auto encode() const { return encode_op<Opcode::INVCALL, true>(on_forward_lo); }
};
template <> struct InvCall<2> {
  u16 on_forward_lo, on_backward_lo;
  constexpr auto encode() const { return encode_op<Opcode::INVCALL, true>(on_forward_lo, on_backward_lo); }
};
template <> struct InvCall<3> {
  SegmentPair on_forward;
  u16 on_backward_lo;
  constexpr auto encode() const {
    return encode_op<Opcode::INVCALL, true>(on_forward.lo, on_backward_lo, on_forward.hi);
  }
};
template <> struct InvCall<4> {
  SegmentPair on_forward, on_backward;
  constexpr auto encode() const {
    return encode_op<Opcode::INVCALL, true>(on_forward.lo, on_backward.lo, on_forward.hi, on_backward.hi);
  }
};

namespace detail {
// The two sync ops encode identically, so share one implementation and let the opcode pick the flavor.
template <Opcode SYNT, std::size_t> struct Syn;

// No packet words: the timestamp is the DS bytes living at DP.
template <Opcode SYNT> struct Syn<SYNT, 0> {
  constexpr auto encode() const { return encode_op<SYNT, true>(); }
};
// Immediate: size word followed by the little-endian timestamp bytes. Carries no prefix words, since the sync ops have
// no target/offset registers to program.
template <Opcode SYNT> struct Syn<SYNT, 1> : ImmediateEncoder<SYNT, Syn<SYNT, 1>> {
  template <typename F> constexpr auto apply_prefix(F &&f) const { return f(); }
};
} // namespace detail

// Absolute timestamp. Data is treated as an unsigned little-endian integer.
template <std::size_t N> using ASyn = detail::Syn<Opcode::ASYN, N>;
// Incremental timestamp. Data is treated as a signed little-endian delta added to the previous timestamp.
template <std::size_t N> using ISyn = detail::Syn<Opcode::ISYN, N>;

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

// Load a single register
template <RegMask R> struct LDR {
  u16 value;
  constexpr auto encode() const { return encode_op<Opcode::LMR, false>((u16)R, value); }
};
using LDMOD1Hi = LDR<RegMask::MOD1_HI>;
using LDMOD1Lo = LDR<RegMask::MOD1_LO>;
using LDMOD2Hi = LDR<RegMask::MOD2_HI>;
using LDMOD2Lo = LDR<RegMask::MOD2_LO>;

namespace detail {
template <Opcode BRT, std::size_t> struct BR;

template <Opcode BRT> struct BR<BRT, 0> {
  constexpr auto encode() const { return encode_op<BRT, true>(); }
};
template <Opcode BRT> struct BR<BRT, 1> {
  u16 displacement_lo;
  constexpr auto encode() const { return encode_op<BRT, true>(displacement_lo); }
};
template <Opcode BRT> struct BR<BRT, 2> {
  SegmentPair displacement;
  constexpr auto encode() const { return encode_op<BRT, true>(displacement.lo, displacement.hi); }
};
} // namespace detail

template <std::size_t N> using NOP = detail::BR<Opcode::NOP, N>;
template <std::size_t N> using BREQ = detail::BR<Opcode::BREQ, N>;
template <std::size_t N> using BRGT = detail::BR<Opcode::BRGT, N>;
template <std::size_t N> using BRGE = detail::BR<Opcode::BRGE, N>;
template <std::size_t N> using BRLT = detail::BR<Opcode::BRLT, N>;
template <std::size_t N> using BRLE = detail::BR<Opcode::BRLE, N>;
template <std::size_t N> using BRNE = detail::BR<Opcode::BRNE, N>;
template <std::size_t N> using BR = detail::BR<Opcode::BR, N>;
template <std::size_t N> using BRF = detail::BR<Opcode::BRF, N>;

template <std::size_t> struct CmpMem;
template <> struct CmpMem<1> {
  u16 dev;
  constexpr auto encode() const { return encode_op<Opcode::CMPMEM, true>(dev); }
};
template <> struct CmpMem<2> {
  u16 dev;
  u16 off_hi;
  constexpr auto encode() const { return encode_op<Opcode::CMPMEM, true>(dev, off_hi); }
};
template <> struct CmpMem<3> {
  u16 dev;
  SegmentPair off;
  constexpr auto encode() const { return encode_op<Opcode::CMPMEM, true>(dev, off.hi, off.lo); }
};
template <> struct CmpMem<4> : ImmediateEncoder<Opcode::CMPMEM, CmpMem<4>> {
  u16 dev;
  SegmentPair off;
  template <typename F> constexpr auto apply_prefix(F &&f) const { return f(dev, off.hi, off.lo); }
};

template <bool X> inline constexpr Opcode SetMemOp = X ? Opcode::SETMEMX : Opcode::SETMEM;

template <bool X, std::size_t> struct SetMem;

template <bool X> struct SetMem<X, 1> {
  u16 access;
  constexpr auto encode() const { return encode_op<SetMemOp<X>, true>(access); }
};
template <bool X> struct SetMem<X, 2> {
  u16 access, dev;
  constexpr auto encode() const { return encode_op<SetMemOp<X>, true>(access, dev); }
};
template <bool X> struct SetMem<X, 3> {
  u16 access, dev, off_hi;
  constexpr auto encode() const { return encode_op<SetMemOp<X>, true>(access, dev, off_hi); }
};
template <bool X> struct SetMem<X, 4> {
  u16 access, dev;
  SegmentPair off;
  constexpr auto encode() const { return encode_op<SetMemOp<X>, true>(access, dev, off.hi, off.lo); }
};
template <bool X> struct SetMem<X, 5> : ImmediateEncoder<SetMemOp<X>, SetMem<X, 5>> {
  u16 access, dev;
  SegmentPair off;
  template <typename F> constexpr auto apply_prefix(F &&f) const { return f(access, dev, off.hi, off.lo); }
};

template <bool X> inline constexpr Opcode SetRegOp = X ? Opcode::SETREGX : Opcode::SETREG;

// Same shape as SetMem, except the ID is two words (register, field) and there is no offset. A field of 0 addresses
// the whole register. The <4> form carries immediate data; the shorter ones take it from DP/DS.
template <bool X, std::size_t> struct SetReg;

template <bool X> struct SetReg<X, 1> {
  u16 access;
  constexpr auto encode() const { return encode_op<SetRegOp<X>, true>(access); }
};
template <bool X> struct SetReg<X, 2> {
  u16 access, reg;
  constexpr auto encode() const { return encode_op<SetRegOp<X>, true>(access, reg); }
};
template <bool X> struct SetReg<X, 3> {
  u16 access, reg, field;
  constexpr auto encode() const { return encode_op<SetRegOp<X>, true>(access, reg, field); }
};
template <bool X> struct SetReg<X, 4> : ImmediateEncoder<SetRegOp<X>, SetReg<X, 4>> {
  u16 access, reg, field;
  template <typename F> constexpr auto apply_prefix(F &&f) const { return f(access, reg, field); }
};

template <std::size_t> struct StepMem;
template <> struct StepMem<1> {
  u16 access;
  constexpr auto encode() const { return encode_op<Opcode::STEPMEM, true>(access); }
};
template <> struct StepMem<2> {
  u16 access, dev;
  constexpr auto encode() const { return encode_op<Opcode::STEPMEM, true>(access, dev); }
};
template <> struct StepMem<3> {
  u16 access, dev, off_hi;
  constexpr auto encode() const { return encode_op<Opcode::STEPMEM, true>(access, dev, off_hi); }
};
template <> struct StepMem<4> {
  u16 access, dev;
  SegmentPair off;
  constexpr auto encode() const { return encode_op<Opcode::STEPMEM, true>(access, dev, off.hi, off.lo); }
};
template <> struct StepMem<5> {
  u16 access, dev;
  SegmentPair off;
  u16 order;
  constexpr auto encode() const { return encode_op<Opcode::STEPMEM, true>(access, dev, off.hi, off.lo, order); }
};

// Deriving from ImmediateEncoder prevents designated initializers, so it spells out a constructor like CmpReg<3>
// The size word is not a member: ImmediateEncoder emits it from the payload it is handed.
template <> struct StepMem<6> : ImmediateEncoder<Opcode::STEPMEM, StepMem<6>> {
  constexpr StepMem<6>(u16 access, u16 dev, SegmentPair off, u16 order)
      : access(access), dev(dev), off(off), order(order) {}
  u16 access, dev;
  SegmentPair off;
  u16 order;
  template <typename F> constexpr auto apply_prefix(F &&f) const { return f(access, dev, off.hi, off.lo, order); }
};

// Same packet as SetReg, minus the X variant: a register reports its own width and byte order, so there is nothing
// for the instruction to say about the destination.
template <std::size_t> struct StepReg;
template <> struct StepReg<1> {
  u16 access;
  constexpr auto encode() const { return encode_op<Opcode::STEPREG, true>(access); }
};
template <> struct StepReg<2> {
  u16 access, reg;
  constexpr auto encode() const { return encode_op<Opcode::STEPREG, true>(access, reg); }
};
template <> struct StepReg<3> {
  u16 access, reg, field;
  constexpr auto encode() const { return encode_op<Opcode::STEPREG, true>(access, reg, field); }
};
template <> struct StepReg<4> : ImmediateEncoder<Opcode::STEPREG, StepReg<4>> {
  constexpr StepReg<4>(u16 access, u16 reg, u16 field) : access(access), reg(reg), field(field) {}
  u16 access, reg, field;
  template <typename F> constexpr auto apply_prefix(F &&f) const { return f(access, reg, field); }
};

// SETMEMX with the offset carried in the payload rather than the packet, so a body that stores to a different
// address every time still encodes identically. See Opcode::SETMEMDX for the data layout.
template <std::size_t> struct SetMemDX;
template <> struct SetMemDX<1> {
  u16 access;
  constexpr auto encode() const { return encode_op<Opcode::SETMEMDX, true>(access); }
};
template <> struct SetMemDX<2> {
  u16 access, dev;
  constexpr auto encode() const { return encode_op<Opcode::SETMEMDX, true>(access, dev); }
};

template <std::size_t> struct ClrMem;
template <> struct ClrMem<1> {
  u16 dev;
  constexpr auto encode() const { return encode_op<Opcode::CLRMEM, true>(dev); }
};
template <> struct ClrMem<2> {
  u16 dev;
  u8 reset;
  constexpr auto encode() const { return encode_op<Opcode::CLRMEM, true>(dev, reset); }
};

template <std::size_t> struct CmpReg;
template <> struct CmpReg<1> {
  u16 reg;
  constexpr auto encode() const { return encode_op<Opcode::CMPREG, true>(reg); }
};
template <> struct CmpReg<2> {
  u16 reg, field;
  constexpr auto encode() const { return encode_op<Opcode::CMPREG, true>(reg, field); }
};
template <> struct CmpReg<3> : ImmediateEncoder<Opcode::CMPREG, CmpReg<3>> {
  constexpr CmpReg<3>(u16 reg, u16 field) : reg(reg), field(field) {}
  u16 reg, field;
  template <typename F> constexpr auto apply_prefix(F &&f) const { return f(reg, field); }
};

// Same ID packet as CMPREG. No field clears whole reg.
template <std::size_t> struct ClrReg;
template <> struct ClrReg<1> {
  u16 reg;
  constexpr auto encode() const { return encode_op<Opcode::CLRREG, true>(reg); }
};
template <> struct ClrReg<2> {
  u16 reg, field;
  constexpr auto encode() const { return encode_op<Opcode::CLRREG, true>(reg, field); }
};

template <std::size_t> struct LDP;
template <> struct LDP<1> {
  u16 DP_lo;
  constexpr auto encode() const { return encode_op<Opcode::LDP, true>(DP_lo); }
};
template <> struct LDP<2> {
  u16 DP_lo, DS;
  constexpr auto encode() const { return encode_op<Opcode::LDP, true>(DP_lo, DS); }
};
template <> struct LDP<3> {
  SegmentPair DP;
  u16 DS;
  constexpr auto encode() const { return encode_op<Opcode::LDP, true>(DP.lo, DS, DP.hi); }
};
struct ACCDP {
  u16 DS;
  constexpr auto encode() const { return encode_op<Opcode::ACCDP, true>(DS); }
};

struct INCDP {
  u16 dp_incr, DS;
  constexpr auto encode() const { return encode_op<Opcode::INCDP, true>(dp_incr, DS); }
};

// MMIO puts offset in the payload rather than the packet for the same reasons as SETMEMDX.
// Omitted the shorter forms because dropping the read/write bit in the 3 data word will be confusing.
// In the future, it might be worth consuming 2 opcodes (one for in, one for out) if this instruction does not compress
// well in practice.
template <std::size_t> struct MMIO;
template <> struct MMIO<3> {
  // false if read, true if write.
  bool read_write;
  u16 access, dev;
  constexpr auto encode() const { return encode_op<Opcode::MMIO, true>(access, dev, (u8)read_write); }
};
} // namespace EncodedOp

// Helpers containing the fully-decoded layout of each opcode.
// The values in each struct will mirror underlying registers, but with the decoder taking care of the bit-cracking and
// register retention on your behalf. Defensive programming suggests default-initialized members due to earlier returns
// in decoder.
namespace DecodedOp {
struct Halt {
  StopCause cause = StopCause::None;
};
struct Ret {};

struct Call {
  SegmentPair next_ip{};
};
// Both targets are always resolved, even when the packet was short enough that one (or both) fell back to the
// fall-through address. execute picks between them on the F bit; nothing else distinguishes the two.
struct InvCall {
  // Called when the machine is stepping forward.
  SegmentPair on_forward;
  // Called when the machine is stepping backward.
  SegmentPair on_backward;
};
struct InvRet {};
// The blaster retains no notion of time, so both sync ops hand the fully-resolved value to whoever is inspecting
// decoded ops between the decode and execute stages. The narrower-than-64-bit encodings have already been extended:
// zero-extended for the absolute timestamp, sign-extended for the delta.
struct ASyn {
  u64 timestamp = 0;
};
struct ISyn {
  i64 delta = 0;
};
struct LMR {
  // decode_lmr returns before touching this on a zero-word packet, and execute_lmr tests it before consulting the
  // (empty) data span, so the default matters.
  tvm::RegMask mask = (tvm::RegMask)0;
  std::span<const u8> data;
  u8 word_count() const { return data.size() / 2; }
  u16 word(u8 i) const {
    auto off = i * 2;
    return (u16)data[off] | ((u16)data[off + 1] << 8);
  }
};
struct BR {
  tvm::ConditionCode condition = (tvm::ConditionCode)0;
  SegmentPair displacement{};
};
// A shared decoding structure for every operation which modifies memory (SETMEM, SETMEMX, SETMEMDX, STEPMEM). Their
// only difference is how the payload is combined with the destination, described by `kind`
struct DeltaMem {
  tvm::Delta kind = tvm::Delta::Assign;
  Operation access{};
  Device::ID target{};
  u32 offset = 0;
  // Where the payload lives.
  SegmentPair data{};
  // Size is the number of bytes of data at the data pointer as well as the size of the destination.
  u16 size = 0;
  // For non-bitwise operations, how to interpret the contents at the destination.
  bits::Order order = bits::Order::LittleEndian;
};

// Same for register ops. Drops both access and order, which are managed through the RegisterScan.
struct DeltaReg {
  tvm::Delta kind = tvm::Delta::Assign;
  RegisterScan::RegisterRef reg{};
  SegmentPair data{};
  u16 size = 0;
};
struct CmpMem {
  Device::ID target{};
  u32 offset = 0;
  SegmentPair data{};
  u16 size = 0;
};
struct ClrMem {
  Device::ID target{};
  u8 data = 0;
};
struct CmpReg {
  RegisterScan::RegisterRef reg{};
  SegmentPair data{};
  u16 size = 0;
};
struct ClrReg {
  RegisterScan::RegisterRef reg{};
};

struct TRADDR {
  Device::ID target{}, source{};
  u32 target_offset = 0, source_offset = 0;
  u32 size = 0;
};
struct LDP {};

struct DPIncr {
  u16 dp_incr = 0;
  u16 DS = 0;
};

struct MMIO {
  // false is read, true is write.
  bool write = false;
  u8 data = 0, size = 0;
  Device::ID target{};
  Operation access{};
  u32 offset = 0;
};

using OpChoice = std::variant<Halt, Ret, Call, InvCall, InvRet, ASyn, ISyn, LMR, BR, DeltaMem, CmpMem, ClrMem, DeltaReg,
                              CmpReg, ClrReg, TRADDR, LDP, DPIncr, MMIO>;
} // namespace DecodedOp
} // namespace tvm
