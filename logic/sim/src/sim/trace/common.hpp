#pragma once
#include "bits/operations/log2.hpp"
#include "sim/api.hpp"
// clang-format off
/*
 *                   00 -- All must end in 00, because these are 8 bit flags
 *                         that are global structures.
 *            0000'0000 -- Empty         Packet
 *            0010'0000 -- Read          Packet w 1B address
 *            0010'0100 -- Addressable   Packet w 1B address, 1B data
 *            0010'1000 -- Addressable   Packet w 1B address, 2B data
 *            0010'1100 -- Addressable   Packet w 1B address, 4B data
 *            0011'0000 -- Addressable   Packet w 1B address, 8B data
 *            0011'0100 -- Addressable   Packet w 1B address, 16B data
 *            0011'1000 -- Addressable   Packet w 1B address, 32B data
 *            0011'1100 -- Addressable   Packet w 1B address, 64B data
 *            0100'0000 -- Read          Packet w 2B address
 *            0100'0100 -- Addressable   Packet w 2B address, 1B data
 *            0100'1000 -- Addressable   Packet w 2B address, 2B data
 *            0100'1100 -- Addressable   Packet w 2B address, 4B data
 *            0101'0000 -- Addressable   Packet w 2B address, 8B data
 *            0101'0100 -- Addressable   Packet w 2B address, 16B data
 *            0101'1000 -- Addressable   Packet w 2B address, 32B data
 *            0101'1100 -- Addressable   Packet w 2B address, 64B data
 *            0110'0000 -- Read          Packet w 4B address
 *            0110'0100 -- Addressable   Packet w 4B address, 1B data
 *            0110'1000 -- Addressable   Packet w 4B address, 2B data
 *            0110'1100 -- Addressable   Packet w 4B address, 4B data
 *            0111'0000 -- Addressable   Packet w 4B address, 8B data
 *            0111'0100 -- Addressable   Packet w 4B address, 16B data
 *            0111'1000 -- Addressable   Packet w 4B address, 32B data
 *            0111'1100 -- Addressable   Packet w 4B address, 64B data
 *            1000'0000 -- Read          Packet w 8B address
 *            1000'0100 -- Addressable   Packet w 8B address, 1B data
 *            1000'1000 -- Addressable   Packet w 8B address, 2B data
 *            1000'1100 -- Addressable   Packet w 8B address, 4B data
 *            1001'0000 -- Addressable   Packet w 8B address, 8B data
 *            1001'0100 -- Addressable   Packet w 8B address, 16B data
 *            1001'1000 -- Addressable   Packet w 8B address, 32B data
 *            1001'1100 -- Addressable   Packet w 8B address, 64B data
 *            1010'0000 -- Write-through Packet w 1B address
 *            1010'0100 -- Write-through Packet w 2B address
 *            1010'1000 -- Write-through Packet w 4B address
 *            1010'1100 -- Write-through Packet w 8B address
 *            1011'0000 --!Default value Packet (unimplemented)
 *            1011'0100 -- Unused
 *            1001'1000 -- Unused
 *            1001'1100 -- Unused
 *            11xx'xx00 -- Unused
 *  xxxx'xxxx'xxxx'xxx1 -- Unused
 */
// clang-format on
namespace sim::trace {
template <typename Data>
concept Pow2 = std::has_single_bit(sizeof(Data));
struct DefaultValue {
  quint8 value;
  static const sim::api::packet::Flags flags() {
    sim::api::packet::Flags ret;
    ret.dyn = 0;
    ret.kind = 0b110'000;
    ret.scope = 0;
    ret.u16 = 0;
    return ret;
  }
};

template <typename Address, Pow2 Data> struct AddressedPayload {
  Address address, length;
  Data data;

  static constexpr sim::api::packet::Flags flags() {
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
};

template <typename Address> struct ReadPayload {
  Address address, length;
  static constexpr sim::api::packet::Flags flags() {
    sim::api::packet::Flags flags;
    flags.dyn = 0;
    flags.kind = ((bits::ceil_log2((sizeof(Address)) + 1) & 0b111) << 3) | 0;
    flags.scope = 0;
    flags.u16 = 0;

    // assert(quint8(flags) != 0);
    return flags;
  }
  inline quint8 payload_size(sim::api::packet::Flags flags) {
    quint8 addrBits = (flags.kind >> 3) & 0b111;
    return 2 * (1 << (addrBits - 1));
  };
};

template <typename Address> struct WriteThroughPayload {
  Address address, length;
  static constexpr sim::api::packet::Flags flags() {
    sim::api::packet::Flags flags;
    flags.dyn = 0;
    flags.kind = 0b101'000 | ((bits::ceil_log2((sizeof(Address)) + 1) & 0b11));
    flags.scope = 0;
    flags.u16 = 0;

    // assert(quint8(flags) != 0);
    return flags;
  }
  inline quint8 payload_size(sim::api::packet::Flags flags) {
    quint8 addrBits = flags.kind & 0b11;
    return 2 * (1 << (addrBits - 1));
  };
};
} // namespace sim::trace
