#include "packet_utils.hpp"

std::size_t sim::trace2::payload_length(const sim::api2::packet::Payload &payload) {
  return std::visit(sim::trace2::detail::PayloadLength{}, payload);
}
std::size_t sim::trace2::packet_payloads_length(sim::api2::trace::PacketIterator iter, bool includeRead) {
  return std::visit(sim::trace2::detail::PacketPayloadsLength{iter, includeRead}, *iter);
}
std::optional<sim::api2::packet::VariableBytes<8>>
sim::trace2::get_address_bytes(const sim::api2::packet::Header &header) {
  return std::visit(detail::GetAddressBytes{}, header);
}

sim::trace2::detail::PacketPayloadsLength::PacketPayloadsLength(sim::api2::trace::PacketIterator iter, bool includeRead)
    : _iter(iter), _includeRead(includeRead) {}

std::size_t
sim::trace2::detail::PacketPayloadsLength::operator()(const sim::api2::packet::header::Clear &header) const {
  return 0;
}

std::size_t
sim::trace2::detail::PacketPayloadsLength::operator()(const sim::api2::packet::header::PureRead &header) const {
  if (_includeRead) return header.payload_len;
  else return 0;
}

void sim::trace2::emitFrameStart(sim::api2::trace::Buffer *tb) {
  tb->writeFragment({sim::api2::frame::header::Trace{}});
}

// Max payload size is a compile time constant, so compute at compile time.
using vb = sim::api2::packet::payload::Variable;
static constexpr auto payload_max_size = vb::N;

void sim::trace2::detail::emit_payloads(sim::api2::trace::Buffer *tb, bits::span<const quint8> buf1,
                                        bits::span<const quint8> buf2) {
  auto data_len = std::min(buf1.size(), buf2.size());
  // Split the data into chunks that are `payload_max_size` bytes long.
  for (int it = 0; it < data_len;) {
    auto payload_len = std::min(data_len - it, payload_max_size);
    bool continues = data_len - it > payload_max_size;
    // Additional payloads needed if it is more than N elements away from data_len.
    auto bytes = api2::packet::VariableBytes<payload_max_size>(payload_len, continues);

    // XOR-encode data to reduce storage by 2x.
    bits::memcpy_xor(bits::span<quint8>{bytes.bytes}, buf1.subspan(it, payload_len), buf2.subspan(it, payload_len));

    api2::packet::payload::Variable payload{std::move(bytes)};
    tb->writeFragment({payload});
    it += payload_len;
  }
}

void sim::trace2::detail::emit_payloads(sim::api2::trace::Buffer *tb, bits::span<const quint8> buf) {
  auto data_len = buf.size();
  // Split the data into chunks that are `payload_max_size` bytes long.
  for (int it = 0; it < data_len;) {
    auto payload_len = std::min(data_len - it, payload_max_size);
    bool continues = data_len - it > payload_max_size;
    // Additional payloads needed if it is more than N elements away from data_len.
    auto bytes = api2::packet::VariableBytes<payload_max_size>(payload_len, continues);

    bits::memcpy(bits::span<quint8>{bytes.bytes}, buf.subspan(it, payload_len));
    api2::packet::payload::Variable payload{std::move(bytes)};
    tb->writeFragment({payload});
    it += payload_len;
  }
}

std::optional<sim::api2::packet::path_t> sim::trace2::get_path(const sim::api2::packet::Header &header) {
  return std::visit(detail::GetHeaderPath{}, header);
}

std::optional<sim::api2::device::ID> sim::trace2::get_id(const sim::api2::packet::Header &header) {

  return std::visit(detail::GetHeaderID{}, header);
}

bool sim::trace2::is_packet_header(const sim::api2::trace::Fragment &f) { return std::visit(IsPacketHeader{}, f); }

sim::api2::packet::Header sim::trace2::as_packet_header(const sim::api2::trace::Fragment &f) {
  return std::visit(AsPacketHeader{}, f);
}

bool sim::trace2::is_packet_payload(const sim::api2::trace::Fragment &f) { return std::visit(IsPacketPayload{}, f); }

sim::api2::packet::Payload sim::trace2::as_packet_payload(const sim::api2::trace::Fragment &f) {
  return std::visit(AsPacketPayload{}, f);
}
