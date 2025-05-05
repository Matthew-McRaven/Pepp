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
#include "sim/api2.hpp"
#include "sim/trace2/packet_utils.hpp"
#include "utils/bits/copy.hpp"

namespace sim::memory {
template <typename Address>
class Input : public api2::memory::Target<Address>, public api2::trace::Source, public api2::trace::Sink {
public:
  using AddressSpan = typename api2::memory::AddressSpan<Address>;
  Input(api2::device::Descriptor device, AddressSpan span, quint8 defaultValue = 0);
  ~Input() = default;
  Input(Input &&other) noexcept = default;
  Input &operator=(Input &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  Input(const Input &) = delete;
  Input &operator=(const Input &) = delete;

  // API v2
  // Target interface
  sim::api2::device::ID deviceID() const override { return _device.id; }
  sim::api2::device::Descriptor device() const override { return _device; }
  AddressSpan span() const override;
  api2::memory::Result read(Address address, bits::span<quint8> dest, api2::memory::Operation op) const override;
  api2::memory::Result write(Address address, bits::span<const quint8> src, api2::memory::Operation op) override;
  void clear(quint8 fill) override;
  void dump(bits::span<quint8> dest) const override;

  // Source interface
  void setBuffer(api2::trace::Buffer *tb) override;
  const api2::trace::Buffer *buffer() const override { return _tb; }
  void trace(bool enabled) override;

  // Sink interface
  bool analyze(api2::trace::PacketIterator iter, sim::api2::trace::Direction) override;

  // Helpers
  QSharedPointer<typename detail::Channel<Address, quint8>::Endpoint> endpoint();
  void setFailPolicy(api2::memory::FailPolicy policy);

private:
  quint8 _fill;
  AddressSpan _span;
  api2::device::Descriptor _device;
  QSharedPointer<detail::Channel<Address, quint8>> _channel;
  QSharedPointer<typename detail::Channel<Address, quint8>::Endpoint> _endpoint;
  api2::memory::FailPolicy _policy = api2::memory::FailPolicy::RaiseError;

  api2::trace::Buffer *_tb = nullptr;
};

template <typename Address>
Input<Address>::Input(api2::device::Descriptor device, AddressSpan span, quint8 defaultValue)
    : _fill(defaultValue), _span(span), _device(device),
      _channel(QSharedPointer<detail::Channel<Address, quint8>>::create(_fill)), _endpoint(_channel->new_endpoint()) {
  if (_span.lower() != _span.upper()) throw std::logic_error("MMI only handles bytes.");
}

template <typename Address> typename Input<Address>::AddressSpan Input<Address>::span() const { return _span; }

template <typename Address> void Input<Address>::clear(quint8 fill) {
  _fill = fill;
  _channel->clear(fill);
  _endpoint->set_to_head();
}

template <typename Address> void Input<Address>::dump(bits::span<quint8> dest) const {
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  auto v = *_endpoint->current_value();
  bits::memcpy(dest, bits::span<const quint8>{&v, sizeof(v)});
}

template <typename Address> void Input<Address>::trace(bool enabled) {
  if (_tb) _tb->trace(_device.id, enabled);
}

template <typename Address>
QSharedPointer<typename detail::Channel<Address, quint8>::Endpoint> Input<Address>::endpoint() {
  return _channel->new_endpoint();
}

template <typename Address> void Input<Address>::setFailPolicy(api2::memory::FailPolicy policy) { _policy = policy; }

template <typename Address>
bool Input<Address>::analyze(api2::trace::PacketIterator iter, api2::trace::Direction direction) {
  auto header = *iter;
  if (!std::visit(sim::trace2::IsSameDevice{_device.id}, header)) return false;
  else if (std::holds_alternative<api2::packet::header::ImpureRead>(header)) {
    // Address is always implicitly 0 since this is a 1-byte port.
    auto hdr = std::get<api2::packet::header::ImpureRead>(header);
    // read() consumes a value via next_value(), which unread will undo.
    if (direction == api2::trace::Direction::Reverse) _endpoint->unread();
    // Forward direction
    // We don't emit multiple payloads, so receiving multiple (or 0) doesn't make sense.
    else if (std::distance(iter.cbegin(), iter.cend()) != 1) return false;
    // Otherwise we are seeing this byte for the first time via the trace.
    // We need to mimic the effect of read() by appending and setting to tail.
    else if (std::holds_alternative<api2::packet::payload::Variable>(*iter.cbegin())) {
      auto payload = std::get<api2::packet::payload::Variable>(*iter.cbegin());
      // Only use the first byte, since this port only has 1 address.
      _endpoint->append_value(payload.payload.bytes[0]);
      _endpoint->set_to_tail();
    }
  } else return false;
  return true;
}

template <typename Address> void Input<Address>::setBuffer(api2::trace::Buffer *tb) { _tb = tb; }

template <typename Address>
api2::memory::Result Input<Address>::read(Address address, bits::span<quint8> dest, api2::memory::Operation op) const {
  using E = api2::memory::Error;
  using Operation = sim::api2::memory::Operation;
  // Length is 1-indexed, address are 0, so must offset by -1.
  auto maxDestAddr = (address + std::max<Address>(0, dest.size() - 1));

  if (address < _span.lower() || maxDestAddr > _span.upper()) throw E(E::Type::OOBAccess, address);
  else if (op.type == Operation::Type::Application || op.type == Operation::Type::BufferInternal) {
    quint8 tmp = *_endpoint->current_value();
    bits::memcpy(dest, bits::span<const quint8>{&tmp, 1});
    // Return early to avoid guard extra guard condition in trace code.
    return {};
  } else if (auto next = _endpoint->next_value(); _policy == api2::memory::FailPolicy::RaiseError && !next) {
    throw E(E::Type::NeedsMMI, address);
  } else if (_policy == api2::memory::FailPolicy::YieldDefaultValue && !next) {
    bits::memcpy(dest, bits::span<const quint8>{&_fill, 1});
  } else {
    quint8 tmp = *next;
    bits::memcpy(dest, bits::span<const quint8>{&tmp, 1});
  }

  // All paths to here are non-application code, and should emit a trace
  if (_tb) _tb->emitMMRead<Address>(_device.id, 0, dest);

  return {};
}

template <typename Address>
api2::memory::Result Input<Address>::write(Address address, bits::span<const quint8> src, api2::memory::Operation op) {
  using E = api2::memory::Error;
  // Length is 1-indexed, address are 0, so must offset by -1.
  auto maxDestAddr = (address + std::max<Address>(0, src.size() - 1));
  if (address < _span.lower() || maxDestAddr > _span.upper()) throw E(E::Type::OOBAccess, address);
  return {};
}

} // namespace sim::memory
