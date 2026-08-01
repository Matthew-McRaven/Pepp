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
// on_true is called when F==1; on_false when F==0. Anything not supplied falls through to the next instruction.
template <std::size_t> struct InvCall;
template <> struct InvCall<0> {
  constexpr auto encode() const { return encode_op<Opcode::INVCALL, true>(); }
};
template <> struct InvCall<1> {
  u16 on_true_lo;
  constexpr auto encode() const { return encode_op<Opcode::INVCALL, true>(on_true_lo); }
};
template <> struct InvCall<2> {
  u16 on_true_lo, on_false_lo;
  constexpr auto encode() const { return encode_op<Opcode::INVCALL, true>(on_true_lo, on_false_lo); }
};
template <> struct InvCall<3> {
  SegmentPair on_true;
  u16 on_false_lo;
  constexpr auto encode() const { return encode_op<Opcode::INVCALL, true>(on_true.lo, on_false_lo, on_true.hi); }
};
template <> struct InvCall<4> {
  SegmentPair on_true, on_false;
  constexpr auto encode() const {
    return encode_op<Opcode::INVCALL, true>(on_true.lo, on_false.lo, on_true.hi, on_false.hi);
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
  // Called when F==1, i.e. after a failed memory access.
  SegmentPair on_true;
  // Called when F==0.
  SegmentPair on_false;
};
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
struct SetMem {
  bool xor_encoded = false;
  Operation access = 0;
  Device::ID target{};
  u32 offset = 0;
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
struct SetReg {
  bool xor_encoded = false;
  Operation access = 0;
  RegisterScan::RegisterRef reg{};
  SegmentPair data{};
  u16 size = 0;
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

using OpChoice = std::variant<Halt, Ret, Call, InvCall, ASyn, ISyn, LMR, BR, SetMem, CmpMem, ClrMem, SetReg, CmpReg,
                              ClrReg, TRADDR, LDP, DPIncr>;
} // namespace DecodedOp
} // namespace tvm