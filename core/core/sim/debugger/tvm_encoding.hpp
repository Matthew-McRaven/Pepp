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

// Create 2/1/0 variants for NOP / BREQ / BRGT / BRGE / BRLT / BRLE / BRNE / BR using the above templates

template <typename... M> auto setmem(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::SETMEM, true>(m...); }
template <typename... M> auto setmemx(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::SETMEMX, true>(m...); }
template <typename... M> auto clrmem(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::CLRMEM, true>(m...); }
template <typename... M> auto setreg(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::SETREG, true>(m...); }
template <typename... M> auto setregx(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::SETREGX, true>(m...); }
template <typename... M> auto clrreg(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::CLRREG, true>(m...); }
template <typename... M> auto traddr(M... m) { return tvm::EncodedOp::encode_op<tvm::Opcode::TRADDR, true>(m...); }

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

struct CmpMem_1 {
  u16 dev;
  constexpr auto encode() const { return encode_op<Opcode::CMPMEM, true>(dev); }
};
struct CmpMem_2 {
  u16 dev;
  u16 off_hi;
  constexpr auto encode() const { return encode_op<Opcode::CMPMEM, true>(dev, off_hi); }
};
struct CmpMem_3 {
  u16 dev;
  SegmentPair off;
  constexpr auto encode() const { return encode_op<Opcode::CMPMEM, true>(dev, off.hi, off.lo); }
};

struct CmpMem_4 {
  u16 dev;
  SegmentPair off;
  template <std::size_t N> constexpr auto encode(const std::array<u16, N> &data) const {
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
      return encode_op<Opcode::CMPMEM, true>(dev, off.hi, off.lo, 2 * (u16)N, data[I]...);
    }(std::make_index_sequence<N>{});
  }
  template <std::size_t N> constexpr auto encode(std::array<u8, N> data) const {
    auto words = pack_bytes(data);
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
      return encode_op<Opcode::CMPMEM, true>(dev, off.hi, off.lo, (u16)N, words[I]...);
    }(std::make_index_sequence<(N / 2) + 1>{});
  }
  template <typename... D>
    requires(std::is_convertible_v<D, u16> && ...)
  constexpr auto encode(D... data) const {
    return encode_op<Opcode::CMPMEM, true>(dev, off.hi, off.lo, 2 * (u16)sizeof...(D), (u16)data...);
  }
};

template <bool xor_encoded> struct SetMem_1 {
  static constexpr Opcode op = xor_encoded ? Opcode::SETMEMX : Opcode::SETMEM;
  u16 access;
  constexpr auto encode() const { return encode_op<op, true>(access); }
};

template <bool xor_encoded> struct SetMem_2 {
  static constexpr Opcode op = xor_encoded ? Opcode::SETMEMX : Opcode::SETMEM;
  u16 access, dev;
  constexpr auto encode() const { return encode_op<op, true>(access, dev); }
};

template <bool xor_encoded> struct SetMem_3 {
  static constexpr Opcode op = xor_encoded ? Opcode::SETMEMX : Opcode::SETMEM;
  u16 access, dev;
  u16 off_hi;
  constexpr auto encode() const { return encode_op<op, true>(access, dev, off_hi); }
};

template <bool xor_encoded> struct SetMem_4 {
  static constexpr Opcode op = xor_encoded ? Opcode::SETMEMX : Opcode::SETMEM;
  u16 access, dev;
  SegmentPair off;
  constexpr auto encode() const { return encode_op<op, true>(access, dev, off.hi, off.lo); }
};

template <bool xor_encoded> struct SetMem_5 {
  static constexpr Opcode op = xor_encoded ? Opcode::SETMEMX : Opcode::SETMEM;
  u16 access, dev;
  SegmentPair off;
  template <std::size_t N> constexpr auto encode(const std::array<u16, N> &data) const {
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
      return encode_op<op, true>(access, dev, off.hi, off.lo, 2 * (u16)N, data[I]...);
    }(std::make_index_sequence<N>{});
  }
  template <std::size_t N> constexpr auto encode(std::array<u8, N> data) const {
    auto words = pack_bytes(data);
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
      return encode_op<op, true>(access, dev, off.hi, off.lo, (u16)N, words[I]...);
    }(std::make_index_sequence<(N / 2) + 1>{});
  }
  template <typename... D>
    requires(std::is_convertible_v<D, u16> && ...)
  constexpr auto encode(D... data) const {
    return encode_op<op, true>(access, dev, off.hi, off.lo, 2 * (u16)sizeof...(D), (u16)data...);
  }
};

struct ClrMem_1 {
  u16 dev;
  constexpr auto encode() const { return encode_op<Opcode::CLRMEM, true>(dev); }
};
struct ClrMem_2 {
  u16 dev;
  u8 reset;
  constexpr auto encode() const { return encode_op<Opcode::CMPMEM, true>(dev, reset); }
};

struct CmpReg_1 {
  u16 reg;
  constexpr auto encode() const { return encode_op<Opcode::CMPREG, true>(reg); }
};
struct CmpReg_2 {
  u16 reg;
  u16 field;
  constexpr auto encode() const { return encode_op<Opcode::CMPREG, true>(reg, field); }
};

struct CmpReg_3 {
  u16 reg;
  u16 field;
  template <std::size_t N> constexpr auto encode(const std::array<u16, N> &data) const {
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
      return encode_op<Opcode::CMPREG, true>(reg, field, 2 * (u16)N, data[I]...);
    }(std::make_index_sequence<N>{});
  }
  template <std::size_t N> constexpr auto encode(std::array<u8, N> data) const {
    auto words = pack_bytes(data);
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
      return encode_op<Opcode::CMPREG, true>(reg, field, (u16)N, words[I]...);
    }(std::make_index_sequence<(N / 2) + 1>{});
  }
  template <typename... D>
    requires(std::is_convertible_v<D, u16> && ...)
  constexpr auto encode(D... data) const {
    return encode_op<Opcode::CMPREG, true>(reg, field, 2 * (u16)sizeof...(D), (u16)data...);
  }
};

// LDPI is variadic width b/c of the way we load data.
// So, give me bytes and I'll encode a packet for you and set DS automatically.

struct LDP_3 {
  SegmentPair DP;
  u16 DS;
  constexpr auto encode() const { return encode_op<Opcode::LDP, true>(DP.lo, DS, DP.hi); }
};
struct LDP_2 {
  u16 DP_lo;
  u16 DS;
  constexpr auto encode() const { return encode_op<Opcode::LDP, true>(DP_lo, DS); }
};
struct LDP_1 {
  u16 DP_lo;
  constexpr auto encode() const { return encode_op<Opcode::LDP, true>(DP_lo); }
};

struct ACCDP_1 {
  u16 DS;
  constexpr auto encode() const { return encode_op<Opcode::ACCDP, true>(DS); }
};

struct INCDP_2 {
  u16 dp_incr;
  u16 DS;
  constexpr auto encode() const { return encode_op<Opcode::INCDP, true>(dp_incr, DS); }
};

} // namespace EncodedOp

// Helpers containing the fully-decoded layout of each opcode.
// The values in each struct will mirror underlying registers, but with the decoder taking care of the bit-cracking and
// register retention on your behalf.
namespace DecodedOp {
struct Halt {
  StopCause cause;
};
struct Ret {};

struct Call {
  SegmentPair next_ip;
};
struct Syn {};
struct LMR {
  tvm::RegMask mask;
  std::span<const u8> data;
  u8 word_count() const { return data.size() / 2; }
  u16 word(u8 i) const {
    auto off = i * 2;
    return (u16)data[off] | ((u16)data[off + 1] << 8);
  }
};
struct BR {
  tvm::ConditionCode condition;
  SegmentPair displacement;
};
struct SetMem {
  bool xor_encoded;
  Operation access;
  Device::ID target;
  u32 offset;
  SegmentPair data;
  u16 size;
};
struct CmpMem {
  Device::ID target;
  u32 offset;
  SegmentPair data;
  u16 size;
};
struct ClrMem {
  Device::ID target;
  u8 data;
};
struct SetReg {
  bool xor_encoded;
  Operation access;
  RegisterScan::RegisterRef reg;
  SegmentPair data;
  u16 size;
};
struct CmpReg {
  RegisterScan::RegisterRef reg;
  SegmentPair data;
  u16 size;
};
struct ClrReg {
  RegisterScan::RegisterRef reg;
};

struct TRADDR {
  Device::ID target, source;
  u32 target_offset, source_offset;
  u32 size;
};
struct LDP {};

struct DPIncr {
  u16 dp_incr;
  u16 DS;
};

using OpChoice =
    std::variant<Halt, Ret, Call, Syn, LMR, BR, SetMem, CmpMem, ClrMem, SetReg, CmpReg, ClrReg, TRADDR, LDP, DPIncr>;
} // namespace DecodedOp
} // namespace tvm