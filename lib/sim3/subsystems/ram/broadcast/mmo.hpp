/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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
#include "./pubsub.hpp"
#include "sim3/api/traced/memory_target.hpp"
#include "sim3/api/traced/trace_endpoint.hpp"
#include "sim3/trace/packet_utils.hpp"
#include "bts/bitmanip/copy.hpp"

namespace sim::memory {
template <typename Address>
class Output : public api2::memory::Target<Address>, public api2::trace::Source, public api2::trace::Sink {
public:
  using AddressSpan = typename api2::memory::AddressSpan<Address>;
  Output(api2::device::Descriptor device, AddressSpan span, quint8 defaultValue = 0);
  ~Output() = default;
  Output(Output &&other) noexcept = default;
  Output &operator=(Output &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  Output(const Output &) = delete;
  Output &operator=(const Output &) = delete;

  // API v2
  // Target interface
  sim::api2::device::ID deviceID() const override { return _device.id; }
  sim::api2::device::Descriptor device() const override { return _device; }
  AddressSpan span() const override;
  api2::memory::Result read(Address address, bits::span<quint8> dest, api2::memory::Operation op) const override;
  api2::memory::Result write(Address address, bits::span<const quint8> src, api2::memory::Operation op) override;
  void clear(quint8 fill) override;
  void dump(bits::span<quint8> dest) const override;

  // Sink interface
  bool analyze(api2::trace::PacketIterator iter, api2::trace::Direction) override;

  // Source interface
  void trace(bool enabled) override;
  void setBuffer(api2::trace::Buffer *tb) override;
  const api2::trace::Buffer *buffer() const override { return _tb; }

  // Helpers
  QSharedPointer<typename detail::Channel<Address, quint8>::Endpoint> endpoint();

private:
  quint8 _fill;
  AddressSpan _span;
  api2::device::Descriptor _device;
  QSharedPointer<typename detail::Channel<Address, quint8>> _channel;
  QSharedPointer<typename detail::Channel<Address, quint8>::Endpoint> _endpoint;

  api2::trace::Buffer *_tb = nullptr;
};

template <typename Address>
Output<Address>::Output(api2::device::Descriptor device, AddressSpan span, quint8 defaultValue)
    : _fill(defaultValue), _span(span), _device(device),
      _channel(QSharedPointer<detail::Channel<Address, quint8>>::create(_fill)), _endpoint(_channel->new_endpoint()) {
  if (_span.lower() != _span.upper()) throw std::logic_error("MMO only works with single byte.");
}

template <typename Address> typename Output<Address>::AddressSpan Output<Address>::span() const { return _span; }

template <typename Address> void Output<Address>::clear(quint8 fill) {
  _channel->clear(fill);
  _endpoint->set_to_head();
}

template <typename Address> void Output<Address>::dump(bits::span<quint8> dest) const {
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  auto v = *_endpoint->current_value();
  bits::memcpy(dest, bits::span<const quint8>{&v, sizeof(v)});
}

template <typename Address> void Output<Address>::trace(bool enabled) {
  if (_tb) _tb->trace(_device.id, enabled);
}

template <typename Address>
QSharedPointer<typename detail::Channel<Address, quint8>::Endpoint> Output<Address>::endpoint() {
  return _channel->new_endpoint();
}

template <typename Address>
bool Output<Address>::analyze(api2::trace::PacketIterator iter, api2::trace::Direction direction) {
  auto header = *iter;
  if (!std::visit(sim::trace2::IsSameDevice{_device.id}, header)) return false;
  else if (std::holds_alternative<api2::packet::header::Write>(header)) {
    // Address is always implicitly 0 since this is a 1-byte port.
    auto hdr = std::get<api2::packet::header::Write>(header);
    (void)hdr;
    if (direction == api2::trace::Direction::Reverse) _endpoint->unwrite();
    // Forward direction
    // We don't emit multiple payloads, so receiving multiple (or 0) doesn't make sense.
    else if (std::distance(iter.cbegin(), iter.cend()) != 1) return false;
    // Otherwise we are seeing this byte for the first time via the trace.
    // We need to mimic the effects of write().
    else if (std::holds_alternative<api2::packet::payload::Variable>(*iter.cbegin())) {
      auto payload = std::get<api2::packet::payload::Variable>(*iter.cbegin());
      // Only use the first byte, since this port only has 1 address.
      _endpoint->append_value(payload.payload.bytes[0]);
    }
  } else return false;
  return true;
}

template <typename Address> void Output<Address>::setBuffer(api2::trace::Buffer *tb) { _tb = tb; }

template <typename Address>
api2::memory::Result Output<Address>::read(Address address, bits::span<quint8> dest, api2::memory::Operation op) const {
  using E = api2::memory::Error;
  using Operation = sim::api2::memory::Operation;
  // Length is 1-indexed, address are 0, so must offset by -1.
  auto maxDestAddr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < _span.lower() || maxDestAddr > _span.upper()) throw E(E::Type::OOBAccess, address);
  // Only emit a trace if the operation isn't related to app-internal state.
  else if (auto end = _endpoint->current_value(); end) {
    if (!(op.type == Operation::Type::Application || op.type == Operation::Type::BufferInternal) && _tb)
      _tb->emitPureRead<Address>(_device.id, 0, dest.size());
    quint8 tmp = *end;
    bits::memcpy(dest, bits::span<const quint8>{&tmp, 1});
  }
  return {};
}

template <typename Address>
api2::memory::Result Output<Address>::write(Address address, bits::span<const quint8> src, api2::memory::Operation op) {
  using E = api2::memory::Error;
  using Operation = sim::api2::memory::Operation;
  // Length is 1-indexed, address are 0, so must offset by -1.
  auto maxDestAddr = (address + std::max<Address>(0, src.size() - 1));
  if (address < _span.lower() || maxDestAddr > _span.upper()) throw E(E::Type::OOBAccess, address);
  // Only emit a trace if the operation isn't related to app-internal state.
  else if (!(op.type == Operation::Type::Application || op.type == Operation::Type::BufferInternal)) {
    if (_tb) _tb->emitMMWrite<Address>(_device.id, 0, src);
    quint8 tmp;
    bits::memcpy(bits::span<quint8>{&tmp, 1}, src);
    _endpoint->append_value(tmp);
  }
  return {};
}

} // namespace sim::memory
