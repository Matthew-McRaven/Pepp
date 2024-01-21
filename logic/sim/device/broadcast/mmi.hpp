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
class Input : public sim::api::memory::Target<Address>, api::trace::Producer {
public:
  using AddressSpan = typename api::memory::AddressSpan<Address>;
  Input(api::device::Descriptor device, AddressSpan span,
        quint8 defaultValue = 0);
  ~Input() = default;
  Input(Input &&other) noexcept = default;
  Input &operator=(Input &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  Input(const Input &) = delete;
  Input &operator=(const Input &) = delete;

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
  void setFailPolicy(api::memory::FailPolicy policy);

private:
  quint8 _fill;
  AddressSpan _span;
  api::device::Descriptor _device;
  QSharedPointer<detail::Channel<Address, quint8>> _channel;
  QSharedPointer<typename detail::Channel<Address, quint8>::Endpoint> _endpoint;
  api::memory::FailPolicy _policy = api::memory::FailPolicy::RaiseError;

  api::trace::Buffer *_tb = nullptr;
};

template <typename Address>
Input<Address>::Input(api::device::Descriptor device, AddressSpan span,
                      quint8 defaultValue)
    : _fill(defaultValue), _span(span),
      _channel(QSharedPointer<detail::Channel<Address, quint8>>::create(_fill)),
      _endpoint(_channel->new_endpoint()) {
  if (_span.minOffset != _span.maxOffset)
    throw std::logic_error("MMI only handles bytes.");
}

template <typename Address>
typename Input<Address>::AddressSpan Input<Address>::span() const {
  return _span;
}

template <typename Address>
api::memory::Result Input<Address>::read(Address address,
                                         bits::span<quint8> dest,
                                         api::memory::Operation op) const {
  // Length is 1-indexed, address are 0, so must convert by -1.
  auto maxDestAddr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < _span.minOffset || maxDestAddr > _span.maxOffset)
    return {.completed = false,
            .pause = true,
            .error = api::memory::Error::OOBAccess};
  auto error = api::memory::Error::Success;
  bool pause = false;

  if (op.effectful && _tb)
    throw std::logic_error("unimplemented tracing");
  bool completed = true;
  if (!op.effectful) {
    quint8 tmp = *_endpoint->current_value();
    bits::memcpy(dest, bits::span<const quint8>{&tmp, 1});
  } else if (auto next = _endpoint->next_value();
             _policy == api::memory::FailPolicy::RaiseError && !next) {
    completed = false;
    error = api::memory::Error::NeedsMMI;
  } else if (_policy == api::memory::FailPolicy::YieldDefaultValue && !next) {
    bits::memcpy(dest, bits::span<const quint8>{&_fill, 1});
  } else {
    quint8 tmp = *next;
    bits::memcpy(dest, bits::span<const quint8>{&tmp, 1});
  }
  return {.completed = completed, .pause = pause, .error = error};
}

template <typename Address>
api::memory::Result Input<Address>::write(Address address,
                                          bits::span<const quint8> src,
                                          api::memory::Operation op) {
  // Length is 1-indexed, address are 0, so must convert by -1.
  auto maxDestAddr = (address + std::max<Address>(0, src.size() - 1));
  if (address < _span.minOffset || maxDestAddr > _span.maxOffset)
    return {.completed = false,
            .pause = true,
            .error = api::memory::Error::OOBAccess};
  return {
      .completed = true,
      .pause = false,
      .error = api::memory::Error::Success,
  };
}

template <typename Address> void Input<Address>::clear(quint8 fill) {
  _fill = fill;
  _endpoint->set_to_head();
}

template <typename Address>
void Input<Address>::dump(bits::span<quint8> dest) const {
  if (dest.size() <= 0)
    throw std::logic_error("dump requires non-0 size");
  auto v = *_endpoint->current_value();
  bits::memcpy(dest, bits::span<const quint8>{&v, sizeof(v)});
}

template <typename Address>
void Input<Address>::setTraceBuffer(api::trace::Buffer *tb) {
  _tb = tb;
}

template <typename Address> void Input<Address>::trace(bool enabled) {
  if (_tb)
    _tb->trace(_device.id, enabled);
}

template <typename Address>
quint8 Input<Address>::packetSize(api::packet::Flags) const {
  throw std::logic_error("unimplemented");
}

template <typename Address>
bool Input<Address>::applyTrace(bits::span<const quint8> payload,
                                api::packet::Flags flags) {
  throw std::logic_error("unimplemented");
}

template <typename Address>
bool Input<Address>::unapplyTrace(bits::span<const quint8> payload,
                                  api::packet::Flags flags) {
  throw std::logic_error("unimplemented");
}

template <typename Address>
QSharedPointer<typename detail::Channel<Address, quint8>::Endpoint>
Input<Address>::endpoint() {
  return _channel->new_endpoint();
}

template <typename Address>
void Input<Address>::setFailPolicy(api::memory::FailPolicy policy) {
  _policy = policy;
}

} // namespace sim::memory
