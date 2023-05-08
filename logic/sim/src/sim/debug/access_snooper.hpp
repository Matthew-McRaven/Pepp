#pragma once
#include "bits/strings.hpp"
#include "sim/api.hpp"
#include "sim/trace/common.hpp"
#include <iostream>

namespace sim::debug {
namespace detail {
struct Decoded {
  bool success = true, write = false;
  quint8 addrBytes;
  quint64 address, length;
};

template <typename Addr>
Decoded decodeRead(void *payload, sim::api::packet::Flags flags) {
  auto casted = static_cast<sim::trace::ReadPayload<Addr> *>(payload);
  return {.success = true,
          .write = false,
          .addrBytes = sizeof(Addr),
          .address = casted->address,
          .length = casted->length};
}
template <typename Addr>
Decoded decodeWriteThrough(void *payload, sim::api::packet::Flags flags) {
  auto casted = static_cast<sim::trace::WriteThroughPayload<Addr> *>(payload);
  return {.success = true,
          .write = true,
          .addrBytes = sizeof(Addr),
          .address = casted->address,
          .length = casted->length};
}
template <typename Addr, typename Data>
Decoded decodeAddressedWrite(void *payload, sim::api::packet::Flags flags) {
  auto casted =
      static_cast<sim::trace::AddressedPayload<Addr, Data> *>(payload);
  return {.success = true,
          .write = true,
          .addrBytes = sizeof(Addr),
          .address = casted->address,
          .length = 0};
}

Decoded decode(void *payload, sim::api::packet::Flags flags) {
  switch ((quint16)flags) {
  case 0b0000'0000: // Empty Packet
    return {.success = true, .address = 0, .length = 0};
  case 0b0010'0000: // Read Packet,           1B Address
    return decodeRead<quint8>(payload, flags);
  case 0b0010'0100: // Write Packet           1B address, 1B data
    return decodeAddressedWrite<quint8, quint8>(payload, flags);
  case 0b0010'1000: // Write Packet           1B address, 2B data
    return decodeAddressedWrite<quint8, quint8[2]>(payload, flags);
  case 0b0010'1100: // Write Packet           1B address, 4B data
    return decodeAddressedWrite<quint8, quint8[4]>(payload, flags);
  case 0b0011'0000: // Write Packet           1B address, 8B data
    return decodeAddressedWrite<quint8, quint8[8]>(payload, flags);
  case 0b0011'0100: // Write Packet           1B address, 16B data
    return decodeAddressedWrite<quint8, quint8[16]>(payload, flags);
  case 0b0011'1000: // Write Packet           1B address, 32B data
    return decodeAddressedWrite<quint8, quint8[32]>(payload, flags);
  case 0b0011'1100: // Write Packet           1B address, 64B data
    return decodeAddressedWrite<quint8, quint8[64]>(payload, flags);
  case 0b0100'0000: // Read Packet,           2B Address
    return decodeRead<quint16>(payload, flags);
  case 0b0100'0100: // Write Packet           2B address, 1B data
    return decodeAddressedWrite<quint16, quint8>(payload, flags);
  case 0b0100'1000: // Write Packet           2B address, 2B data
    return decodeAddressedWrite<quint16, quint8[2]>(payload, flags);
  case 0b0100'1100: // Write Packet           2B address, 4B data
    return decodeAddressedWrite<quint16, quint8[4]>(payload, flags);
  case 0b0101'0000: // Write Packet           2B address, 8B data
    return decodeAddressedWrite<quint16, quint8[8]>(payload, flags);
  case 0b0101'0100: // Write Packet           2B address, 16B data
    return decodeAddressedWrite<quint16, quint8[16]>(payload, flags);
  case 0b0101'1000: // Write Packet           2B address, 32B data
    return decodeAddressedWrite<quint16, quint8[32]>(payload, flags);
  case 0b0101'1100: // Write Packet           2B address, 64B data
    return decodeAddressedWrite<quint16, quint8[64]>(payload, flags);
  case 0b0110'0000: // Read Packet,           4B Address
    return decodeRead<quint32>(payload, flags);
  case 0b0110'0100: // Write Packet           4B address, 1B data
    return decodeAddressedWrite<quint32, quint8>(payload, flags);
  case 0b0110'1000: // Write Packet           4B address, 2B data
    return decodeAddressedWrite<quint32, quint8[2]>(payload, flags);
  case 0b0110'1100: // Write Packet           4B address, 4B data
    return decodeAddressedWrite<quint32, quint8[4]>(payload, flags);
  case 0b0111'0000: // Write Packet           4B address, 8B data
    return decodeAddressedWrite<quint32, quint8[8]>(payload, flags);
  case 0b0111'0100: // Write Packet           4B address, 16B data
    return decodeAddressedWrite<quint32, quint8[16]>(payload, flags);
  case 0b0111'1000: // Write Packet           4B address, 32B data
    return decodeAddressedWrite<quint32, quint8[32]>(payload, flags);
  case 0b0111'1100: // Write Packet           4B address, 64B data
    return decodeAddressedWrite<quint32, quint8[64]>(payload, flags);
  case 0b1000'0000: // Read Packet,           8B Address
    return decodeRead<quint64>(payload, flags);
  case 0b1000'0100: // Write Packet           8B address, 1B data
    return decodeAddressedWrite<quint64, quint8>(payload, flags);
  case 0b1000'1000: // Write Packet           8B address, 2B data
    return decodeAddressedWrite<quint64, quint8[2]>(payload, flags);
  case 0b1000'1100: // Write Packet           8B address, 4B data
    return decodeAddressedWrite<quint64, quint8[4]>(payload, flags);
  case 0b1011'0000: // Write Packet           8B address, 8B data
    return decodeAddressedWrite<quint64, quint8[8]>(payload, flags);
  case 0b1011'0100: // Write Packet           8B address, 16B data
    return decodeAddressedWrite<quint64, quint8[16]>(payload, flags);
  case 0b1011'1000: // Write Packet           8B address, 32B data
    return decodeAddressedWrite<quint64, quint8[32]>(payload, flags);
  case 0b1011'1100: // Write Packet           8B address, 64B data
    return decodeAddressedWrite<quint64, quint8[64]>(payload, flags);
  case 0b1010'0000: // Writethrough packet,   1B address
    return decodeWriteThrough<quint8>(payload, flags);
  case 0b1010'0100: // Writethrough packet,   2B address
    return decodeWriteThrough<quint16>(payload, flags);
  case 0b1010'1000: // Writethrough packet,   4B address
    return decodeWriteThrough<quint32>(payload, flags);
  case 0b1010'1100: // Writethrough packet,   8B address
    return decodeWriteThrough<quint64>(payload, flags);
  }
  return {.success = false};
};
const auto gs = sim::api::memory::Operation{
    .speculative = true,
    .kind = sim::api::memory::Operation::Kind::data,
    .effectful = false,
};
} // namespace detail
template <typename Address>
class AccessSnooper : public sim::api::trace::Analyzer {
public:
  explicit AccessSnooper(sim::api::memory::Target<Address> *target);
  // Analyzer interface
  bool analyze(void *payload, api::packet::Flags flags) override;
  bool unanalyze(void *payload, api::packet::Flags flags) override;
  FilterArgs filter() const override;

private:
  void printData(detail::Decoded decoded, bool dir);
  sim::api::memory::Target<Address> *target;
};

template <typename Address>
bool sim::debug::AccessSnooper<Address>::analyze(void *payload,
                                                 api::packet::Flags flags) {
  auto decoded = detail::decode(payload, flags);
  printData(decoded, false);
  return decoded.success;
}

template <typename Address>
// We already printed access to console... can't really "undo" that.
bool sim::debug::AccessSnooper<Address>::unanalyze(void *payload,
                                                   api::packet::Flags flags) {
  auto decoded = detail::decode(payload, flags);
  printData(decoded, true);
  return decoded.success;
}

template <typename Address>
sim::api::trace::Analyzer::FilterArgs
sim::debug::AccessSnooper<Address>::filter() const {
  return {};
}

template <typename Address>
void AccessSnooper<Address>::printData(detail::Decoded decoded, bool dir) {
  if (decoded.success && decoded.length > 0) {
    std::cout << u"[%1]"_qs
                     .arg(QString::number(decoded.address, 16),
                          decoded.addrBytes * 2)
                     .toStdString()
              << (decoded.write ^ dir ? ">" : "<");
    char str[64];
    quint8 data[16];
    quint64 it = 0;
    while (it < decoded.length) {
      auto count = std::min<quint64>(sizeof(data), decoded.length - it);
      target->read(decoded.address + count, data, count, detail::gs);
      bits::bytesToAsciiHex(str, sizeof(str), data, sizeof(data), false);
      std::cout.write(str, count * 2);
      it += count;
    }
    std::cout << "\n";
  }
}
} // namespace sim::debug