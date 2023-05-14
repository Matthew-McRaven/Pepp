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
  const quint8 *constData() const;

private:
  quint8 _fill;
  AddressSpan _span;
  api::device::Descriptor _device;
  QVector<quint8> _data;

  api::trace::Buffer *_tb = nullptr;
};

template <typename Address>
sim::memory::Dense<Address>::Dense(api::device::Descriptor device,
                                   AddressSpan span, quint8 fill)
    : _fill(fill), _span(span), _device(device) {
  _data.fill(_fill,
             _span.maxOffset - _span.minOffset + 1); // Resizes before filling.
}

template <typename Address>
typename sim::memory::Dense<Address>::AddressSpan
sim::memory::Dense<Address>::span() const {
  return _span;
}

template <typename Address>
sim::api::memory::Result
sim::memory::Dense<Address>::read(Address address, std::span<quint8> dest,
                                  api::memory::Operation op) const {
  // Length is 1-indexed, address are 0, so must convert by -1.
  auto maxDestAddr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < _span.minOffset || maxDestAddr > _span.maxOffset)
    return {.completed = false,
            .pause = true,
            .error = api::memory::Error::OOBAccess};
  auto error = api::memory::Error::Success;
  bool pause = false;

  if (op.effectful && _tb) {
    using Read = sim::trace::ReadPayload<Address>;
    api::trace::Buffer::Guard<true> guard(
        _tb, sizeof(api::packet::Packet<Read>), _device.id, Read::flags());
    if (guard) {
      auto payload = Read{.address = address, .length = Address(dest.size())};
      auto it = new (guard.data())
          api::packet::Packet<Read>(_device.id, payload, Read::flags());
    }
  }
  bits::memcpy(dest, bits::span<const quint8>{_data.constData(),
                                              std::size_t(_data.size())}
                         .subspan(address - _span.minOffset));
  return {.completed = true, .pause = pause, .error = error};
}

namespace detail {
template <typename Address, typename T>
using payload = sim::trace::AddressedPayload<Address, T>;
template <typename Address>
std::tuple<quint8, api::packet::Flags> info(quint8 length) {
  using namespace api::packet;
  api::packet::Flags flags;
  auto bit_ceil = std::bit_ceil(length);
  quint8 size = 0;
  switch (bit_ceil) {
  case 1:
    size = sizeof(payload<Address, quint8>);
    flags = payload<Address, quint8>::flags();
    break;
  case 2:
    size = sizeof(payload<Address, quint8[2]>);
    flags = payload<Address, quint8[2]>::flags();
    break;
  case 4:
    size = sizeof(payload<Address, quint8[4]>);
    flags = payload<Address, quint8[4]>::flags();
    break;
  case 8:
    size = sizeof(payload<Address, quint8[8]>);
    flags = payload<Address, quint8[8]>::flags();
    break;
  case 16:
    size = sizeof(payload<Address, quint8[16]>);
    flags = payload<Address, quint8[16]>::flags();
    break;
  case 32:
    size = sizeof(payload<Address, quint8[32]>);
    flags = payload<Address, quint8[32]>::flags();
    break;
  case 64:
    size = sizeof(payload<Address, quint8[64]>);
    flags = payload<Address, quint8[64]>::flags();
    break;
  }
  return {size, flags};
}

template <typename Address, typename dtype>
quint8 *init_help(void *packet, quint16 dataLen, Address address,
                  api::device::ID id, api::packet::Flags flags) {
  using namespace api::packet;
  using pkt = Packet<payload<Address, dtype>>;
  pkt *ptr = new (packet) pkt(id, flags);
  ptr->payload.address = address;
  ptr->payload.length = dataLen;
  if constexpr (std::is_pointer_v<std::decay_t<dtype>>)
    return ptr->payload.data;
  else
    return &(ptr->payload.data);
}

template <typename Address>
quint8 *init(void *packet, quint16 dataLen, Address address, api::device::ID id,
             api::packet::Flags flags) {
  auto bit_ceil = std::bit_ceil(dataLen);
  switch (bit_ceil) {
  case 1:
    return init_help<Address, quint8>(packet, dataLen, address, id, flags);
  case 2:
    return init_help<Address, quint8[2]>(packet, dataLen, address, id, flags);
  case 4:
    return init_help<Address, quint8[4]>(packet, dataLen, address, id, flags);
  case 8:
    return init_help<Address, quint8[8]>(packet, dataLen, address, id, flags);
  case 16:
    return init_help<Address, quint8[16]>(packet, dataLen, address, id, flags);
  case 32:
    return init_help<Address, quint8[32]>(packet, dataLen, address, id, flags);
  case 64:
    return init_help<Address, quint8[64]>(packet, dataLen, address, id, flags);
  }
  throw std::logic_error("impossible");
}
} // namespace detail

template <typename Address>
sim::api::memory::Result sim::memory::Dense<Address>::write(
    Address address, bits::span<const quint8> src, api::memory::Operation op) {
  // Length is 1-indexed, address are 0, so must convert by -1.
  auto maxDestAddr = (address + std::max<Address>(0, src.size() - 1));
  if (address < _span.minOffset || maxDestAddr > _span.maxOffset)
    return {.completed = false,
            .pause = true,
            .error = api::memory::Error::OOBAccess};
  auto error = api::memory::Error::Success;
  bool pause = false;

  auto offset = address - _span.minOffset;
  bool success = true, sync = false;
  auto dataSpan =
      bits::span<quint8>{_data.data(), std::size_t(_data.size())}.subspan(
          offset);
  if (op.effectful && _tb) {
    // Attempt to allocate space in the buffer for local trace packet.
    auto [size, flags] = detail::info<Address>(src.size());
    api::trace::Buffer::Guard<false> guard(_tb, size, _device.id, flags);
    // Even with success we might get nullptr, in which case the Buffer is
    // telling us it doesn't want our trace.
    if (guard) {
      auto dest =
          detail::init(guard.data(), src.size(), offset, _device.id, flags);
      bits::memcpy_xor(bits::span<quint8>{dest, src.size()}, dataSpan, src);
    }
  }
  auto data =
      bits::span<quint8>{_data.data(), std::size_t(_data.length())}.subspan(
          offset);
  if (success)
    bits::memcpy(dataSpan, src);
  return {.completed = success, .pause = pause, .error = error};
}

template <typename Address>
void sim::memory::Dense<Address>::clear(quint8 fill) {
  this->_fill = fill;
  this->_data.fill(this->_fill);
}

template <typename Address>
void Dense<Address>::dump(bits::span<quint8> dest) const {
  if (dest.size() <= 0)
    throw std::logic_error("dump requires non-0 size");
  bits::memcpy(dest, bits::span<const quint8>{_data.constData(),
                                              std::size_t(_data.size())});
}

template <typename Address>
void sim::memory::Dense<Address>::setTraceBuffer(api::trace::Buffer *tb) {
  this->_tb = tb;
}

template <typename Address> void Dense<Address>::trace(bool enabled) {
  if (this->_tb)
    _tb->trace(_device.id, enabled);
}

template <typename Address>
quint8 Dense<Address>::packetSize(api::packet::Flags flags) const {
  throw std::logic_error("unimplemented");
}

template <typename Address>
bool sim::memory::Dense<Address>::applyTrace(bits::span<const quint8> payload,
                                             api::packet::Flags flags) {
  throw std::logic_error("unimplemented");
  return false;
}

template <typename Address>
bool sim::memory::Dense<Address>::unapplyTrace(bits::span<const quint8> payload,
                                               api::packet::Flags flags) {
  throw std::logic_error("unimplemented");
  return false;
}

template <typename Address>
const quint8 *sim::memory::Dense<Address>::constData() const {
  return _data.constData();
}
} // namespace sim::memory
