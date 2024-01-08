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
#include "./pubsub.hpp"
#include "bits/operations/copy.hpp"
#include "sim/api.hpp"

namespace sim::memory {
template <typename Address>
class Output : public sim::api::memory::Target<Address>, api::trace::Producer {
public:
  using AddressSpan = typename api::memory::Target<Address>::AddressSpan;
  Output(api::device::Descriptor device, AddressSpan span,
         quint8 defaultValue = 0);
  ~Output() = default;
  Output(Output &&other) noexcept = default;
  Output &operator=(Output &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  Output(const Output &) = delete;
  Output &operator=(const Output &) = delete;

  // Target interface
  AddressSpan span() const override;
  api::memory::Result read(Address address, bits::span<quint8> dest,
                           api::memory::Operation op) const override;
  api::memory::Result write(Address address, bits::span<const quint8> src,
                            api::memory::Operation op) override;
  void clear(quint8 fill) override;
  void dump(bits::span<quint8> dest) const override;

  // Producer interface
  void setTraceBuffer(api::trace::Buffer *tb) override;
  void trace(bool enabled) override;
  quint8 packetSize(api::packet::Flags) const override;
  bool applyTrace(bits::span<const quint8> payload,
                  api::packet::Flags flags) override;
  bool unapplyTrace(bits::span<const quint8> payload,
                    api::packet::Flags flags) override;

  // Helpers
  QSharedPointer<typename detail::Channel<Address, quint8>::Endpoint>
  endpoint();

private:
  quint8 _fill;
  AddressSpan _span;
  api::device::Descriptor _device;
  QSharedPointer<typename detail::Channel<Address, quint8>> _channel;
  QSharedPointer<typename detail::Channel<Address, quint8>::Endpoint> _endpoint;

  api::trace::Buffer *_tb = nullptr;
};

template <typename Address>
Output<Address>::Output(api::device::Descriptor device, AddressSpan span,
                        quint8 defaultValue)
    : _fill(defaultValue), _span(span), _device(device),
      _channel(QSharedPointer<detail::Channel<Address, quint8>>::create(_fill)),
      _endpoint(_channel->new_endpoint()) {
  if (_span.minOffset != _span.maxOffset)
    throw std::logic_error("MMO only works with single byte.");
}

template <typename Address>
typename Output<Address>::AddressSpan Output<Address>::span() const {
  return _span;
}

template <typename Address>
api::memory::Result Output<Address>::read(Address address,
                                          bits::span<quint8> dest,
                                          api::memory::Operation op) const {
  // Length is 1-indexed, address are 0, so must convert by -1.
  auto maxDestAddr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < _span.minOffset || maxDestAddr > _span.maxOffset)
    return {.completed = false,
            .pause = true,
            .error = api::memory::Error::OOBAccess};
  // Copy last-written value, so that memory dumps look right.
  if (auto end = _endpoint->current_value(); end) {
    quint8 tmp = *end;
    bits::memcpy(dest, bits::span<const quint8>{&tmp, 1});
  }
  return {
      .completed = true,
      .pause = false,
      .error = api::memory::Error::Success,
  };
}

template <typename Address>
api::memory::Result Output<Address>::write(Address address,
                                           bits::span<const quint8> src,
                                           api::memory::Operation op) {
  // Length is 1-indexed, address are 0, so must convert by -1.
  auto maxDestAddr = (address + std::max<Address>(0, src.size() - 1));
  if (address < _span.minOffset || maxDestAddr > _span.maxOffset)
    return {.completed = false,
            .pause = true,
            .error = api::memory::Error::OOBAccess};
  auto error = api::memory::Error::Success;
  bool pause = false;

  if (op.effectful && _tb)
    throw std::logic_error("no tracing yet");
  if (op.effectful) {
    quint8 tmp;
    bits::memcpy(bits::span<quint8>{&tmp, 1}, src);
    _endpoint->append_value(tmp);
  }
  return {.completed = true, .pause = pause, .error = error};
}

template <typename Address> void Output<Address>::clear(quint8 fill) {
  _endpoint->set_to_head();
}

template <typename Address>
void Output<Address>::dump(bits::span<quint8> dest) const {
  if (dest.size() <= 0)
    throw std::logic_error("dump requires non-0 size");
  auto v = *_endpoint->current_value();
  bits::memcpy(dest, bits::span<const quint8>{&v, sizeof(v)});
}

template <typename Address>
void Output<Address>::setTraceBuffer(api::trace::Buffer *tb) {
  _tb = tb;
}

template <typename Address> void Output<Address>::trace(bool enabled) {
  if (_tb)
    _tb->trace(_device.id, enabled);
}

template <typename Address>
quint8 Output<Address>::packetSize(api::packet::Flags) const {
  throw std::logic_error("unimplemented");
}

template <typename Address>
bool Output<Address>::applyTrace(bits::span<const quint8> payload,
                                 api::packet::Flags flags) {
  throw std::logic_error("unimplemented");
}

template <typename Address>
bool Output<Address>::unapplyTrace(bits::span<const quint8> payload,
                                   api::packet::Flags flags) {
  throw std::logic_error("unimplemented");
}

template <typename Address>
QSharedPointer<typename detail::Channel<Address, quint8>::Endpoint>
Output<Address>::endpoint() {
  return _channel->new_endpoint();
}

} // namespace sim::memory