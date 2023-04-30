#pragma once
#include "bits/operations/copy.hpp"
#include "sim/api.hpp"

namespace sim::memory {
template <typename Address>
class Dense : public api::Memory::Target<Address>, api::Trace::Producer {
public:
  using AddressSpan = typename api::Memory::Target<Address>::AddressSpan;
  Dense(api::Device::Descriptor device, AddressSpan span,
        quint8 defaultValue = 0);
  ~Dense() = default;
  Dense(Dense &&other) noexcept = default;
  Dense &operator=(Dense &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  Dense(const Dense &) = delete;
  Dense &operator=(const Dense &) = delete;

  // Target interface
  AddressSpan span() const override;
  api::Memory::Result read(Address address, quint8 *dest, quint8 length,
                           api::Memory::Operation op) const override;
  api::Memory::Result write(Address address, const quint8 *src, quint8 length,
                            api::Memory::Operation op) override;
  void clear(quint8 fill) override;
  void setInterposer(api::Memory::Interposer<Address> *inter) override;

  // Producer interface
  void setTraceBuffer(api::Trace::Buffer *tb) override;
  bool applyTrace(void *trace) override;
  bool unapplyTrace(void *trace) override;

  // Helpers
  const quint8 *constData() const;

private:
  quint8 _fill;
  AddressSpan _span;
  api::Device::Descriptor _device;
  QVector<quint8> _data;

  api::Memory::Interposer<Address> *_inter = nullptr;
  api::Trace::Buffer *_tb = nullptr;

  union Packets {
    // Must have ctor, or compiler complains about non-trivial members.
    Packets() { memset(this, 0, sizeof(*this)); };
    api::Packet::Packet<quint8> u8;
    api::Packet::Packet<quint8[2]> u16;
    api::Packet::Packet<quint8[4]> u32;
    api::Packet::Packet<quint8[8]> u64;
    api::Packet::Packet<quint8 *> un;
  };
};

template <typename Address>
sim::memory::Dense<Address>::Dense(api::Device::Descriptor device,
                                   AddressSpan span, quint8 fill)
    : _fill(fill), _span(span), _device(device) {
  _data.fill(_fill, _span.length); // Resizes before filling.
}

template <typename Address>
typename sim::memory::Dense<Address>::AddressSpan
sim::memory::Dense<Address>::span() const {
  return _span;
}

template <typename Address>
sim::api::Memory::Result
sim::memory::Dense<Address>::read(Address address, quint8 *dest, quint8 length,
                                  api::Memory::Operation op) const {

  // Subtract rather than add to avoid potential overflow.
  // Equivalently address + qMin(0, length-1) < _span.minOffset+_span.length-1
  // length is 1-indexed, address are 0, so must convert by -1.
  auto maxDestAddr = (address + qMax(0, length - 1));
  if (address < _span.minOffset ||
      maxDestAddr - qMax(0, _span.length - 1) > _span.minOffset)
    return {.completed = false,
            .advance = false,
            .pause = true,
            .sync = false,
            .error = api::Memory::Error::OOBAccess};
  auto error = api::Memory::Error::Success;
  bool pause = false;
  if (op.effectful && _inter) {
    auto res = _inter->tryRead(address, length, op);
    if (res == api::Memory::Interposer<Address>::Result::Breakpoint) {
      pause = true;
      error = api::Memory::Error::Breakpoint;
    }
  }
  bits::memcpy(dest, _data.constData() + (address - _span.minOffset), length);
  return {.completed = true,
          .advance = true,
          .pause = pause,
          .sync = false,
          .error = error};
}

template <typename Address>
sim::api::Memory::Result
sim::memory::Dense<Address>::write(Address address, const quint8 *src,
                                   quint8 length, api::Memory::Operation op) {
  // Subtract rather than add to avoid potential overflow.
  // Equivalently address + qMin(0, length-1) < _span.minOffset+_span.length-1
  // length is 1-indexed, address are 0, so must convert by -1.
  auto maxDestAddr = (address + qMax(0, length - 1));
  if (address < _span.minOffset ||
      maxDestAddr - qMax(0, _span.length - 1) > _span.minOffset)
    return {.completed = false,
            .advance = false,
            .pause = true,
            .sync = false,
            .error = api::Memory::Error::OOBAccess};
  auto error = api::Memory::Error::Success;
  bool pause = false;
  if (op.effectful && _inter) {
    auto res = _inter->tryWrite(address, src, length, op);
    if (res == api::Memory::Interposer<Address>::Result::Breakpoint) {
      pause = true;
      error = api::Memory::Error::Breakpoint;
    }
  }
  bool success = true, sync = false;
  if (_tb) {
    Packets trace;
    quint64 buf = 0;
    if (length <= 8)
      bits::memcpy(&buf, _data.constData() + (address - _span.minOffset),
                   length);
    switch (length) {
    case 1:
      trace.u8 = api::Packet::Packet<quint8>{_device.id, *(quint8 *)&buf,
                                             api::Packet::Flags{}};
      break;
    case 2:
      trace.u16 = api::Packet::Packet<quint8[2]>{_device.id, (quint8 *)&buf,
                                                 api::Packet::Flags{}};
      break;
    case 4:
      trace.u32 = api::Packet::Packet<quint8[4]>{_device.id, (quint8 *)&buf,
                                                 api::Packet::Flags{}};
      break;
    case 8:
      trace.u64 = api::Packet::Packet<quint8[8]>{_device.id, (quint8 *)&buf,
                                                 api::Packet::Flags{}};
      break;
    default:
      throw std::logic_error(
          "Can't handle write whose length is not in [1,2,4,8].");
      break;
    }
    auto res = _tb->push(&trace);
    switch (res) {
    case api::Trace::Buffer::Status::Success:
      break;
    case api::Trace::Buffer::Status::OverflowAndRetry:
      success = false;
      [[fallthrough]];
    case api::Trace::Buffer::Status::OverflowAndSuccess:
      sync = true;
    }
  }
  if (success)
    memcpy(_data.data() + (address - _span.minOffset), src, length);
  return {.completed = success,
          .advance = success,
          .pause = pause,
          .sync = sync,
          .error = error};
}

template <typename Address>
void sim::memory::Dense<Address>::clear(quint8 fill) {
  this->_fill = fill;
  this->_data.fill(this->_fill);
}

template <typename Address>
void sim::memory::Dense<Address>::setInterposer(
    api::Memory::Interposer<Address> *inter) {
  this->_inter = inter;
}

template <typename Address>
void sim::memory::Dense<Address>::setTraceBuffer(api::Trace::Buffer *tb) {
  this->_tb = tb;
}

template <typename Address>
bool sim::memory::Dense<Address>::applyTrace(void *trace) {
  return false;
}

template <typename Address>
bool sim::memory::Dense<Address>::unapplyTrace(void *trace) {
  return false;
}

template <typename Address>
const quint8 *sim::memory::Dense<Address>::constData() const {
  return _data.constData();
}
} // namespace sim::memory
