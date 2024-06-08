#pragma once
#include "sim/api2.hpp"
#include "bits/operations/copy.hpp"
namespace sim::trace2 {
// TODO: should check that the struct is a packet header.
template <typename T>
concept HasDevice = requires(T t) {
  { t.device } -> std::convertible_to<decltype(t.device)>;
};
class IsSameDevice {
public:

    IsSameDevice(sim::api2::device::ID device): _device(device) {};
    template <HasDevice Header>
    bool operator()(const Header& header) const {return header.device == _device;};
    bool operator()(const auto& header) const {return false;}
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
    for (auto it = _iter.cbegin(); it != _iter.cend(); ++it)
      acc += payload_length(*it);
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

template <typename T> std::optional<T> get_address(const sim::api2::packet::Header &header) {
  return std::visit(detail::GetAddress<T>{}, header);
}

namespace detail {
void emit_payloads(sim::api2::trace::Buffer* tb,
                   bits::span<const quint8> buf1, bits::span<const quint8> buf2);
void emit_payloads(sim::api2::trace::Buffer* tb,
                   bits::span<const quint8> buf1);
} // namespace detail

void emitFrameStart(sim::api2::trace::Buffer* tb);

template <typename Address>
void emitWrite(sim::api2::trace::Buffer* tb, sim::api2::device::ID id,
           Address address, bits::span<const quint8> src, bits::span<quint8> dest) {
    using vb = decltype(api2::packet::header::Write::address);
    auto address_bytes = vb::from_address<Address>(address);
    auto header = api2::packet::header::Write{.device = id, .address = address_bytes};
    // Don't write payloads if the buffer rejected the packet header.
    if (tb->writeFragmentWithPath(header))
      detail::emit_payloads(tb, src, dest);
}

// Generate a Write packet. Bytes will not be XOR encoded.
// A write to a MM port appends to the state of that port.
// We do not need to know previous value, since the pub/sub system records it.
template <typename Address>
void emitMMWrite(sim::api2::trace::Buffer* tb, sim::api2::device::ID id,
               Address address, bits::span<const quint8> src) {
    using vb = decltype(api2::packet::header::Write::address);
    auto header = api2::packet::header::Write{.device = id,
                                              .address = vb::from_address(address)};
    // Don't write payloads if the buffer rejected the packet header.
    if (tb->writeFragmentWithPath(header))
      detail::emit_payloads(tb, src);
}

template <typename Address>
void emitPureRead(sim::api2::trace::Buffer* tb, sim::api2::device::ID id,
           Address address, Address len) {
    using vb = decltype(api2::packet::header::PureRead::address);
    auto header = api2::packet::header::PureRead{.device = id, .payload_len = len,
                                                 .address = vb::from_address(address)};
    tb->writeFragmentWithPath(header);
}

// Generate a ImpureRead packet. Bytes will not be XOR encoded.
// We do not need to know previous value, since the pub/sub system records it.
template <typename Address>
void emitMMRead(sim::api2::trace::Buffer* tb, sim::api2::device::ID id,
                 Address address, bits::span<const quint8> src) {
    using vb = decltype(api2::packet::header::Write::address);
    auto header = api2::packet::header::ImpureRead{.device = id,
                                              .address = vb::from_address(address)};
    // Don't write payloads if the buffer rejected the packet header.
    if (tb->writeFragmentWithPath(header))
      detail::emit_payloads(tb, src);
}

} // sim::trace2
