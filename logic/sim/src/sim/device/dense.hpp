#pragma once
#include "bits/operations/copy.hpp"
#include "sim/api.hpp"
#include "sim/trace/common.hpp"

namespace sim::memory {
template <typename Address>
class Dense : public api::memory::Target<Address>, api::trace::Producer {
public:
  using AddressSpan = typename api::memory::Target<Address>::AddressSpan;
  Dense(api::device::Descriptor device, AddressSpan span,
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
  api::memory::Result read(Address address, quint8 *dest, quint8 length,
                           api::memory::Operation op) const override;
  api::memory::Result write(Address address, const quint8 *src, quint8 length,
                            api::memory::Operation op) override;
  void clear(quint8 fill) override;
  void setInterposer(api::memory::Interposer<Address> *inter) override;

  // Producer interface
  void setTraceBuffer(api::trace::Buffer *tb) override;
  quint8 packetSize(api::packet::Flags) const override;
  bool applyTrace(void *trace) override;
  bool unapplyTrace(void *trace) override;

  // Helpers
  const quint8 *constData() const;

private:
  quint8 _fill;
  AddressSpan _span;
  api::device::Descriptor _device;
  QVector<quint8> _data;

  api::memory::Interposer<Address> *_inter = nullptr;
  api::trace::Buffer *_tb = nullptr;
};

template <typename Address>
sim::memory::Dense<Address>::Dense(api::device::Descriptor device,
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
sim::api::memory::Result
sim::memory::Dense<Address>::read(Address address, quint8 *dest, quint8 length,
                                  api::memory::Operation op) const {

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
  bits::memcpy(dest, _data.constData() + (address - _span.minOffset), length);
  return {.completed = true,
          .advance = true,
          .pause = pause,
          .sync = false,
          .error = error};
}

namespace detail {
template <typename Address, typename T>
using payload = bits::trace::AddressedPayload<Address, T>;
template <typename Address>
api::trace::Buffer::Status alloc(void **dest, quint8 length,
                                 api::trace::Buffer *tb) {
  using namespace api::packet;
  auto bit_ceil = std::bit_ceil(length);
  api::trace::Buffer::Status ret;
  quint8 size = 0;
  switch (bit_ceil) {
  case 1:
    size = sizeof(payload<Address, quint8>);
    break;
  case 2:
    size = sizeof(payload<Address, quint8[2]>);
    break;
  case 4:
    size = sizeof(payload<Address, quint8[4]>);
    break;
  case 8:
    size = sizeof(payload<Address, quint8[8]>);
    break;
  case 16:
    size = sizeof(payload<Address, quint8[16]>);
    break;
  case 32:
    size = sizeof(payload<Address, quint8[32]>);
    break;
  case 64:
    size = sizeof(payload<Address, quint8[64]>);
    break;
  }
  ret = tb->request(size, dest);
  return ret;
}

template <typename Address, typename dtype>
quint8 *init_help(void *packet, quint16 dataLen, Address address,
                  api::device::ID id) {
  using namespace api::packet;
  using pkt = Packet<payload<Address, dtype>>;
  pkt *ptr = new (packet) pkt(id, bits::trace::flags<Address, dtype>());
  ptr->payload.address = address;
  if constexpr (std::is_pointer_v<std::decay_t<dtype>>)
    return ptr->payload.data;
  else
    return &(ptr->payload.data);
}

template <typename Address>
quint8 *init(void *packet, quint16 dataLen, Address address,
             api::device::ID id) {
  auto bit_ceil = std::bit_ceil(dataLen);
  switch (bit_ceil) {
  case 1:
    return init_help<Address, quint8>(packet, dataLen, address, id);
  case 2:
    return init_help<Address, quint8[2]>(packet, dataLen, address, id);
  case 4:
    return init_help<Address, quint8[4]>(packet, dataLen, address, id);
  case 8:
    return init_help<Address, quint8[8]>(packet, dataLen, address, id);
  case 16:
    return init_help<Address, quint8[16]>(packet, dataLen, address, id);
  case 32:
    return init_help<Address, quint8[32]>(packet, dataLen, address, id);
  case 64:
    return init_help<Address, quint8[64]>(packet, dataLen, address, id);
  }
  throw std::logic_error("impossible");
}
} // namespace detail

template <typename Address>
sim::api::memory::Result
sim::memory::Dense<Address>::write(Address address, const quint8 *src,
                                   quint8 length, api::memory::Operation op) {
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
            .error = api::memory::Error::OOBAccess};
  auto error = api::memory::Error::Success;
  bool pause = false;
  if (op.effectful && _inter) {
    auto res = _inter->tryWrite(address, src, length, op);
    if (res == api::memory::Interposer<Address>::Result::Breakpoint) {
      pause = true;
      error = api::memory::Error::Breakpoint;
    }
  }
  bool success = true, sync = false;
  if (op.effectful && _tb) {
    // Attempt to allocate space in the buffer for local trace packet.
    void *packet = nullptr;
    auto res = detail::alloc<Address>(&packet, length, _tb);
    switch (res) {
    case api::trace::Buffer::Status::Success:
      break;
    case api::trace::Buffer::Status::OverflowAndRetry:
      success = false;
      [[fallthrough]];
    case api::trace::Buffer::Status::OverflowAndSuccess:
      sync = true;
    }
    // Even with success we might get nullptr, in which case the Buffer is
    // telling us it doesn't want our trace.
    if (packet != nullptr) {
      auto offset = address - _span.minOffset;
      auto dest = detail::init(packet, length, offset, _device.id);
      bits::memcpy(dest, _data.constData() + offset, length);
      bits::memcpy_xor(dest, dest, src, length);
    }
  }
  if (success)
    bits::memcpy(_data.data() + (address - _span.minOffset), src, length);
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
    api::memory::Interposer<Address> *inter) {
  this->_inter = inter;
}

template <typename Address>
void sim::memory::Dense<Address>::setTraceBuffer(api::trace::Buffer *tb) {
  this->_tb = tb;
}

template <typename Address>
quint8 Dense<Address>::packetSize(api::packet::Flags flags) const {
  throw std::logic_error("unimplemented");
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
