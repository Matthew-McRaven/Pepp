/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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
#include <QtCore>
#include <zpp_bits.h>
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/span.hpp"

// A fragment is one of the data classes in packet::header, packet::payload, or
// frame::header. A packet is a one packet::header fragment followed by 0 or
// more packet::payload fragments A frame is a frame::header fragment followed
// by 0 or more packets.
namespace sim::api2::packet {
// Stack-allocated variable length array of bytes. When serialized, it will only
// occupy the number of bytes specified in data_len, not the size as specified
// by N. One use for this class is to serialize addresses, which may be 1-8
// bytes long. These variable-length addresses allow us to use the same packet
// for all memory accesses. For example, in Pep/10 a register is a 1 byte
// address, where main memory is 2. Our old trace system needed a packet type
// for each address size. The variable-length byte-array allows this to be
// expressed in a single packet type.
template <size_t N> struct VariableBytes {
  explicit VariableBytes() {
    this->len = 0;
    bytes.fill(0);
  }
  explicit VariableBytes(quint8 len, bool continues = false) {
    this->len = (len & len_mask()) | (continues ? 0x80 : 0x00);
    bytes.fill(0);
  }

  VariableBytes(quint8 len, bits::span<const quint8> src, bool continues = false) {
    this->len = (len & len_mask()) | (continues ? 0x80 : 0x00);
    bits::memcpy(bits::span<quint8>{bytes.data(), len}, src);
  }

  template <std::unsigned_integral Address> static VariableBytes from_address(Address address) {
    constexpr auto len = sizeof(address);
    // Copy address bytes into bytes array.
    return VariableBytes(len, bits::span<const quint8>((quint8 *)&address, len));
  }

  template <std::unsigned_integral Address> Address to_address() const {
    Address address = 0;
    auto addr_span = bits::span<quint8>((quint8 *)&address, sizeof(address));
    // Rely on memcpy to perform bounds checking between len and
    // sizeof(address).
    bits::memcpy(addr_span, bits::span<const quint8>{bytes.data(), len});
    return address;
  }

  constexpr static zpp::bits::errc serialize(auto &archive, auto &self) {
    // Serialize array manually to avoid allocating extra 0's in the bit stream.
    // Must also de-serialize manually, otherwise archive will advance the
    // position by the allocated size of the array, not the "used" size.
    if (archive.kind() == zpp::bits::kind::out) {
      // Mask out flag bits before checking max size.
      auto len = self.len & len_mask();
      if (len > N) return zpp::bits::errc(std::errc::value_too_large);
      // Write out length + flags.
      zpp::bits::errc errc = archive(self.len);
      if (errc.code != std::errc()) return errc;
      else if (self.len == 0) return errc;

      // Let compiler deduce [const quint8] vs [quint8].
      auto span = std::span(self.bytes.data(), len);
      return archive(zpp::bits::bytes(span, len));
    }
    // Only allow reading into nonconst objects
    else if (archive.kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      zpp::bits::errc errc = archive(self.len);
      auto len = self.len & len_mask();
      if (errc.code != std::errc()) return errc;
      // Ignore flag bits in bounds check
      else if (len > N) return zpp::bits::errc(std::errc::value_too_large);
      else if (len == 0) return errc;

      // We serialized the length ourselves. If we pass array_view directly,
      // size will be serialzed again.
      auto array_view = bits::span<quint8>(self.bytes.data(), len);
      return archive(zpp::bits::bytes(array_view, array_view.size_bytes()));
    } else if (archive.kind() == zpp::bits::kind::in) {
      const char *const e = "Can't read into const";
      qCritical(e);
      throw std::logic_error(e);
    }
    const char *const e = "Unreachable";
    qCritical(e);
    throw std::logic_error(e);
  }

  bool continues() const { return len & ~len_mask(); }

  static constexpr quint8 len_mask() { return 0x7f; }

  // Ensure that we don't clobber flags with a too-large array.
  static_assert(N < len_mask() + 1);

  // High order bit is used for "continues" flag.
  quint8 len = 0;
  std::array<quint8, N> bytes = {0};

  // Used to allow membership in std::set.
  auto operator<=>(const VariableBytes<N> &other) const {
    if (len != other.len) return len <=> other.len;
    if (auto res = memcmp(bytes.data(), other.bytes.data(), len); res < 0) return std::strong_ordering::less;
    else if (res > 0) return std::strong_ordering::greater;
    else return std::strong_ordering::equal;
  }
  // Equality operator
  bool operator==(const VariableBytes<N> &other) const = default;
};

using device_id_t = zpp::bits::varint<quint16>;
// Type used to associate a packet with the series of devices it traverses.
// A path of 0 implies no address translation.
// See: api2/path.hpp
using path_t = quint16;
enum class DeltaEncoding : quint8 {
  XOR, // The old and new values are XOR'ed together
};

namespace header {
struct Clear {
  device_id_t device = 0;
  static constexpr std::size_t N = 8;
  VariableBytes<N> value = VariableBytes<N>{0};
};

struct PureRead {
  device_id_t device = 0;
  zpp::bits::varint<path_t> path = 0;
  zpp::bits::varint<quint64> payload_len = 0;
  static constexpr std::size_t N = 8;
  VariableBytes<N> address = VariableBytes<N>{0};
};

// MUST be followed by 1+ payloads.
struct ImpureRead {
  device_id_t device = 0;
  zpp::bits::varint<path_t> path = 0;
  static constexpr std::size_t N = 8;
  VariableBytes<N> address = VariableBytes<N>{0};
};

// MUST be followed by 1+ payload.
struct Write {
  device_id_t device = 0;
  zpp::bits::varint<path_t> path = 0;
  static constexpr std::size_t N = 8;
  VariableBytes<N> address = VariableBytes<N>{0};
};

// MUST be followed by 1+ payload containing addend.
// Can be either a ++ or -- operation, so treat payload as signed.
struct Increment {
  device_id_t device = 0;
  zpp::bits::varint<quint64> payload_len = 0;
  static constexpr std::size_t N = 8;
  VariableBytes<N> address = VariableBytes<N>{0};
};

} // namespace header
// If you add a type, update Fragment trace/buffer.hpp
using Header =
    std::variant<std::monostate, header::Clear, header::PureRead, header::ImpureRead, header::Write, header::Increment>;

template <typename T>
concept HasPath = requires(T t) {
  { t.path } -> std::convertible_to<path_t>;
};

namespace payload {
// Successive payloads belong to the same packet.
struct Variable {
  static constexpr std::size_t N = 32;
  using Bytes = VariableBytes<N>;
  VariableBytes<N> payload = VariableBytes<N>{0};
};
} // namespace payload
// If you add a type, update Fragment trace/buffer.hpp
using Payload = std::variant<std::monostate, payload::Variable>;
} // namespace sim::api2::packet
