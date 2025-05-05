/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <iostream>
#include "utils/bits/strings.hpp"
#include "sim/api.hpp"
#include "sim/trace/common.hpp"

namespace sim::debug {
namespace detail {
struct Decoded {
  bool success = true, write = false;
  quint8 addrBytes;
  quint64 address, length;
};

template <typename Addr> Decoded decodeRead(bits::span<const quint8> payload, sim::api2::packet::Flags flags) {
  Q_ASSERT(sizeof(sim::trace::ReadPayload<Addr>) <= payload.size());
  auto casted = reinterpret_cast<const sim::trace::ReadPayload<Addr> *>(payload.data());
  return {
      .success = true, .write = false, .addrBytes = sizeof(Addr), .address = casted->address, .length = casted->length};
}
template <typename Addr> Decoded decodeWriteThrough(bits::span<const quint8> payload, sim::api2::packet::Flags flags) {
  Q_ASSERT(sizeof(sim::trace::WriteThroughPayload<Addr>) <= payload.size());
  auto casted = reinterpret_cast<const sim::trace::WriteThroughPayload<Addr> *>(payload.data());
  return {
      .success = true, .write = true, .addrBytes = sizeof(Addr), .address = casted->address, .length = casted->length};
}
template <typename Addr, typename Data>
Decoded decodeAddressedWrite(bits::span<const quint8> payload, sim::api2::packet::Flags flags) {
  Q_ASSERT(sizeof(sim::trace::AddressedPayload<Addr, Data>) <= payload.size());
  auto casted = reinterpret_cast<const sim::trace::AddressedPayload<Addr, Data> *>(payload.data());
  return {.success = true, .write = true, .addrBytes = sizeof(Addr), .address = casted->address, .length = 0};
}

Decoded decode(bits::span<const quint8> payload, sim::api2::packet::Flags flags) {
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
const auto gs = sim::api2::memory::Operation{
    .speculative = true,
    .kind = sim::api2::memory::Operation::Kind::data,
    .effectful = false,
};
} // namespace detail
template <typename Address> class AccessSnooper : public sim::api2::trace::Analyzer {
public:
  explicit AccessSnooper(sim::api2::memory::Target<Address> *target);
  // Analyzer interface
  bool analyze(bits::span<const quint8> payload, api2::packet::Flags flags) override;
  bool unanalyze(bits::span<const quint8> payload, api2::packet::Flags flags) override;
  FilterArgs filter() const override;

private:
  void printData(detail::Decoded decoded, bool dir);
  sim::api2::memory::Target<Address> *target;
};

template <typename Address>
bool sim::debug::AccessSnooper<Address>::analyze(bits::span<const quint8> payload, api2::packet::Flags flags) {
  auto decoded = detail::decode(payload, flags);
  printData(decoded, false);
  return decoded.success;
}

template <typename Address>
// We already printed access to console... can't really "undo" that.
bool sim::debug::AccessSnooper<Address>::unanalyze(bits::span<const quint8> payload, api2::packet::Flags flags) {
  auto decoded = detail::decode(payload, flags);
  printData(decoded, true);
  return decoded.success;
}

template <typename Address> sim::api2::trace::Analyzer::FilterArgs sim::debug::AccessSnooper<Address>::filter() const {
  return {};
}

template <typename Address> void AccessSnooper<Address>::printData(detail::Decoded decoded, bool dir) {
  using namespace Qt::StringLiterals;
  if (decoded.success && decoded.length > 0) {
    std::cout << u"[%1]"_s.arg(QString::number(decoded.address, 16), decoded.addrBytes * 2).toStdString()
              << (decoded.write ^ dir ? ">" : "<");
    char str[64];
    std::span<char> strSpan = str;
    quint8 data[16];
    std::span<quint8> dataSpan = data;
    quint64 it = 0;
    while (it < decoded.length) {
      auto count = std::min<quint64>(sizeof(data), decoded.length - it);
      target->read(decoded.address + count, data, count, detail::gs);
      bits::bytesToAsciiHex(strSpan, dataSpan, {});
      std::cout.write(str, count * 2);
      it += count;
    }
    std::cout << "\n";
  }
}
} // namespace sim::debug
