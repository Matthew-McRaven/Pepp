#pragma once
#include "sim/api2.hpp"
#include "sim/utils.hpp"
#include "utils/bits/copy.hpp"
namespace sim::trace2 {

template <typename T>
concept PacketHeader =
    is_variant_member<T, sim::api2::packet::Header>::value && !std::is_same<T, std::monostate>::value;
template <typename T>
concept NotPacketHeader =
    !is_variant_member<T, sim::api2::packet::Header>::value || std::is_same<T, std::monostate>::value;

template <typename T>
concept PacketPayload =
    is_variant_member<T, sim::api2::packet::Payload>::value && !std::is_same<T, std::monostate>::value;
template <typename T>
concept NotPacketPayload =
    !is_variant_member<T, sim::api2::packet::Payload>::value || std::is_same<T, std::monostate>::value;

struct IsPacketHeader {
  template <PacketHeader T> bool operator()(const T &hdr) { return true; }
  template <NotPacketHeader T> bool operator()(const T &hdr) { return false; }
};
struct AsPacketHeader {
  template <PacketHeader T> api2::packet::Header operator()(const T &hdr) { return hdr; }
  template <NotPacketHeader T> api2::packet::Header operator()(const T &hdr) { return {}; }
};

struct IsPacketPayload {
  template <PacketPayload T> bool operator()(const T &) const { return true; }
  template <NotPacketPayload T> bool operator()(const T &) { return false; }
};
struct AsPacketPayload {
  template <PacketPayload T> api2::packet::Payload operator()(const T &hdr) const { return hdr; }
  template <NotPacketPayload T> api2::packet::Payload operator()(const T &) { return {}; }
};

bool is_packet_header(const sim::api2::trace::Fragment &f);
api2::packet::Header as_packet_header(const sim::api2::trace::Fragment &f);
bool is_packet_payload(const sim::api2::trace::Fragment &f);
api2::packet::Payload as_packet_payload(const sim::api2::trace::Fragment &f);

// TODO: should check that the struct is a packet header.
template <typename T>
concept HasDevice = requires(T t) {
  { t.device } -> std::convertible_to<decltype(t.device)>;
};
class IsSameDevice {
public:
  IsSameDevice(sim::api2::device::ID device) : _device(device){};
  template <HasDevice Header> bool operator()(const Header &header) const { return header.device == _device; };
  bool operator()(const auto &header) const { return false; }

private:
  sim::api2::device::ID _device;
};

// Number of actual bytes in a single payload.
std::size_t payload_length(const sim::api2::packet::Payload &payload);
// Number of bytes in all payloads following a packet header.
std::size_t packet_payloads_length(api2::trace::PacketIterator iter, bool includeRead = true);

namespace detail {
// Return the address of a packet as type T, otherwise return nullopt if not present.
template <typename T>
concept HasAddress = requires(T t) {
  { t.address } -> std::convertible_to<decltype(t.address)>;
};
using Addr_t = quint16;
template <typename T> class GetAddress {
public:
  template <HasAddress Header> std::optional<T> operator()(const Header &header) const {
    return std::make_optional<T>(header.address.template to_address<T>());
  };
  std::optional<T> operator()(const auto &header) const { return std::nullopt; }
};

class GetAddressBytes {
public:
  using Bytes = api2::packet::VariableBytes<8>;
  template <HasAddress Header> std::optional<Bytes> operator()(const Header &header) const {
    return std::make_optional<Bytes>(header.address);
  };
  std::optional<Bytes> operator()(const auto &header) const { return std::nullopt; }
};

class GetHeaderPath {
public:
  template <sim::api2::packet::HasPath Header>
  std::optional<api2::packet::path_t> operator()(const Header &header) const {
    return header.path;
  };
  std::optional<api2::packet::path_t> operator()(const auto &header) const { return std::nullopt; }
};

class GetHeaderID {
public:
  template <HasDevice Header> std::optional<api2::device::ID> operator()(const Header &header) const {
    return header.device;
  };
  std::optional<api2::device::ID> operator()(const auto &header) const { return 0; }
};

// Return the number of bytes in all payloads following a packet header.
// If includeRead is true, PureRead payload lengths are returned from packets. Otherwise, 0 is returned.
// No other packet types are impacted. It is meant to help determine the set of addresses written to over a simulation.
class PacketPayloadsLength {
public:
  PacketPayloadsLength(sim::api2::trace::PacketIterator iter, bool includeRead = true);
  ;
  std::size_t operator()(const sim::api2::packet::header::PureRead &header) const;
  std::size_t operator()(const sim::api2::packet::header::Clear &header) const;
  template <typename Header> std::size_t operator()(const Header &header) const {
    std::size_t acc = 0;
    sim::api2::packet::Payload vb;
    for (auto it = _iter.cbegin(); it != _iter.cend(); ++it) acc += payload_length(*it);
    return acc;
  }

private:
  sim::api2::trace::PacketIterator _iter;
  bool _includeRead;
};

// Return the number of bytes in a single payload.
struct PayloadLength {
  std::size_t operator()(const sim::api2::packet::payload::Variable &p) const { return p.payload.len; }
  template <typename T> std::size_t operator()(const T &p) const { return 0; }
};
} // namespace detail

std::optional<api2::packet::VariableBytes<8>> get_address_bytes(const sim::api2::packet::Header &header);
std::optional<api2::packet::path_t> get_path(const sim::api2::packet::Header &header);
std::optional<api2::device::ID> get_id(const sim::api2::packet::Header &header);

template <typename T> std::optional<T> get_address(const sim::api2::packet::Header &header) {
  return std::visit(detail::GetAddress<T>{}, header);
}

} // namespace sim::trace2
