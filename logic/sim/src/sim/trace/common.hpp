#pragma once
#include "bits/operations/log2.hpp"
#include "sim/api.hpp"
namespace bits::trace {
template <typename Data>
concept Pow2 = std::has_single_bit(sizeof(Data));
template <typename Address, Pow2 Data> struct AddressedPayload {
  Address address;
  Data data;
};

template <typename Address, Pow2 Data>
constexpr sim::api::packet::Flags flags() {
  // Only have 3 bits to store pow^2, so we can only stor up to 64.
  static_assert(sizeof(Data) <= 64);
  sim::api::packet::Flags flags;
  flags.dyn = 0;
  flags.kind = ((bits::ceil_log2((sizeof(Address)) + 1) & 0b111) << 3) |
               ((bits::ceil_log2(sizeof(Data)) + 1) & 0b111);
  flags.scope = 0;
  flags.u16 = 0;

  // assert(quint8(flags) != 0);
  return flags;
}

inline quint8 payload_size(sim::api::packet::Flags flags) {
  quint8 addrBits = (flags.kind >> 3) & 0b111;
  quint8 dataBits = flags.kind & 0b111;
  return (1 << (addrBits - 1)) + (1 << (dataBits - 1));
};
} // namespace bits::trace
