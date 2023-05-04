#pragma once
#include "./pubsub.hpp"
#include "bits/operations/copy.hpp"
#include "sim/api.hpp"

namespace sim::memory {
template <typename Address>
class Input : public sim::api::memory::Target<Address>, api::trace::Producer {
  using AddressSpan = typename api::memory::Target<Address>::AddressSpan;
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
  api::memory::Result read(Address address, quint8 *dest, quint8 length,
                           api::memory::Operation op) const override;
  api::memory::Result write(Address address, const quint8 *src, quint8 length,
                            api::memory::Operation op) override;
  void clear(quint8 fill) override;
  void setInterposer(api::memory::Interposer<Address> *inter) override;

  // Producer interface
  void setTraceBuffer(api::trace::Buffer *tb) override;
  void trace(bool enabled) override;
  quint8 packetSize(api::packet::Flags) const override;
  bool applyTrace(void *trace) override;
  bool unapplyTrace(void *trace) override;

  // Helpers
  QSharedPointer<typename detail::Channel<Address, quint8>::Endpoint>
  endpoint();

private:
  quint8 _fill;
  AddressSpan _span;
  api::device::Descriptor _device;
  QSharedPointer<typename detail::Channel<Address, quint8>::Channel> _channel;
  QSharedPointer<typename detail::Channel<Address, quint8>::Endpoint> _endpoint;

  api::memory::Interposer<Address> *_inter = nullptr;
  api::trace::Buffer *_tb = nullptr;
};

template <typename Address>
Input<Address>::Input(api::device::Descriptor device, AddressSpan span,
                      quint8 defaultValue)
    : _fill(defaultValue), _span(span),
      _channel(QSharedPointer<detail::Channel<Address, quint8>>::create(_fill),
               _endpoint(_channel->endpoint())) {
  if (_span.minOffset != _span.maxOffset)
    throw std::logic_error("MMI only handles bytes.");
}

template <typename Address>
typename Input<Address>::AddressSpan Input<Address>::span() const {
  return _span;
}

template <typename Address>
api::memory::Result Input<Address>::read(Address address, quint8 *dest,
                                         quint8 length,
                                         api::memory::Operation op) const {
  // Length is 1-indexed, address are 0, so must convert by -1.
  auto maxDestAddr = (address + qMax(0, length - 1));
  if (address < _span.minOffset || maxDestAddr > _span.maxOffset)
    return {.completed = false,
            .pause = true,
            .error = api::memory::Error::OOBAccess};
  auto error = api::memory::Error::Success;
  bool pause = false;
  if (op.effectful && _inter) {
    auto res = _inter->tryRead(address, length, op);
    if (res == api::memory::Interposer<Address>::Result::Breakpoint) {
      pause = true;
      error = api::memory::Error::Breakpoint;
    }
  }

  if (op.effectful && _tb)
    throw std::logic_error("unimplemented tracing");
  bool completed = true;
  if (!op.effectful) {
    quint8 tmp = _endpoint->current_value();
    bits::memcpy(dest, &tmp, 1);
  } else if (auto next = _endpoint->next_value(); !next) {
    completed = false;
    error = api::memory::Error::NeedsMMI;
  } else {
    quint8 tmp = *next;
    bits::memcpy(dest, &tmp, 1);
  }
  return {.completed = completed, .pause = pause, .error = error};
}

template <typename Address>
api::memory::Result Input<Address>::write(Address address, const quint8 *src,
                                          quint8 length,
                                          api::memory::Operation op) {
  // Length is 1-indexed, address are 0, so must convert by -1.
  auto maxDestAddr = (address + qMax(0, length - 1));
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
  _endpoint->clear();
}

template <typename Address>
void Input<Address>::setInterposer(api::memory::Interposer<Address> *inter) {
  _inter = inter;
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

template <typename Address> bool Input<Address>::applyTrace(void *trace) {
  throw std::logic_error("unimplemented");
}

template <typename Address> bool Input<Address>::unapplyTrace(void *trace) {
  throw std::logic_error("unimplemented");
}

} // namespace sim::memory
