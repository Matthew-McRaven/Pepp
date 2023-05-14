#pragma once
#include "bits/operations/copy.hpp"
#include "sim/api.hpp"
#include "sim/trace/common.hpp"
#include <iomanip>
#include <iostream>
namespace sim::memory {
template <typename Address>
class SimpleBus : public api::memory::Target<Address>, api::trace::Producer {
public:
  using AddressSpan = typename api::memory::Target<Address>::AddressSpan;
  SimpleBus(api::device::Descriptor device, AddressSpan span);
  ~SimpleBus() = default;
  SimpleBus(SimpleBus &&other) noexcept = default;
  SimpleBus &operator=(SimpleBus &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  SimpleBus(const SimpleBus &) = delete;
  SimpleBus &operator=(const SimpleBus &) = delete;

  // Target interface
  AddressSpan span() const override;
  api::memory::Result read(Address address, quint8 *dest, Address length,
                           api::memory::Operation op) const override;
  api::memory::Result write(Address address, const quint8 *src, Address length,
                            api::memory::Operation op) override;
  void clear(quint8 fill) override;
  void setInterposer(sim::api::memory::Interposer<Address> *inter) override;
  void dump(quint8 *dest, qsizetype maxLen) const override;

  // Producer interface
  void setTraceBuffer(api::trace::Buffer *tb) override;
  void trace(bool enabled) override;
  quint8 packetSize(api::packet::Flags flags) const override;
  bool applyTrace(bits::span<const quint8> payload,
                  api::packet::Flags flags) override;
  bool unapplyTrace(bits::span<const quint8> payload,
                    api::packet::Flags flags) override;

  // Bus API
  void pushFrontTarget(AddressSpan at, api::memory::Target<Address> *target);
  api::memory::Target<Address> *deviceAt(Address address);

private:
  AddressSpan _span;
  api::device::Descriptor _device;
  api::memory::Interposer<Address> *_inter = nullptr;
  api::trace::Buffer *_tb = nullptr;

  struct Region {
    AddressSpan span;
    api::memory::Target<Address> *target;
  };
  Region regionAt(Address address);
  template <typename Data, bool w>
  api::memory::Result access(Address address, Data data, Address length,
                             api::memory::Operation op) const {
    // Length is 1-indexed, address are 0, so must convert by -1.
    auto maxDestAddr = (address + qMax(0, length - 1));
    if (address < _span.minOffset || maxDestAddr > _span.maxOffset)
      return {.completed = false,
              .pause = true,
              .error = api::memory::Error::OOBAccess};
    bool pause = false;
    auto error = api::memory::Error::Success;
    Address offset = 0;
    while (length > 0) {
      Region region = {{}, nullptr};
      // Inline body of regionAt manually, so that I can violate const with
      // mutable.
      for (auto reg : _regions) {
        if (reg.span.minOffset <= (address + offset) &&
            (address + offset) <= reg.span.maxOffset) {
          region = reg;
          break;
        }
      }

      if (region.target == nullptr) {
        error = api::memory::Error::Unmapped;
        break;
      }

      // Compute how many bytes we can read without OOB'ing on the device.
      auto devSpan = region.target->span();
      auto devLength = devSpan.maxOffset - devSpan.minOffset + 1;
      auto usableLength = std::min<qsizetype>(length, devLength);

      // Convert bus address => device address
      auto busToDev = (address + offset) - region.span.minOffset;
      api::memory::Result acc;
      if constexpr (w)
        acc = region.target->write(busToDev, data + offset, usableLength, op);
      else
        acc = region.target->read(busToDev, data + offset, usableLength, op);

      // Forward any errors from child device.
      pause |= acc.pause;
      if (!acc.completed) {
        error = acc.error;
        break;
      }

      offset += usableLength;
      length -= usableLength;
    }
    return {.completed = error == api::memory::Error::Success,
            .pause = pause,
            .error = error};
  }
  // Marked as mutable so that access can be const even when it is doing a
  // write. Reduces amount of code by 2x because read/write use same
  // implementation now.
  mutable QList<Region> _regions = {};
};

template <typename Address>
SimpleBus<Address>::SimpleBus(api::device::Descriptor device, AddressSpan span)
    : _span(span), _device(device) {}

template <typename Address>
typename SimpleBus<Address>::AddressSpan SimpleBus<Address>::span() const {
  return _span;
}

template <typename Address>
api::memory::Result SimpleBus<Address>::read(Address address, quint8 *dest,
                                             Address length,
                                             api::memory::Operation op) const {
  auto ret = access<quint8 *, false>(address, dest, length, op);
  if (op.effectful && _tb) {
    using Read = sim::trace::ReadPayload<Address>;
    api::trace::Buffer::Guard<true> guard(
        _tb, sizeof(api::packet::Packet<Read>), _device.id, Read::flags());
    if (guard) {
      auto payload = Read{.address = address, .length = length};
      auto it = new (guard.data())
          api::packet::Packet<Read>(_device.id, payload, Read::flags());
    }
  }
  return ret;
}
template <typename Address>
api::memory::Result SimpleBus<Address>::write(Address address,
                                              const quint8 *src, Address length,
                                              api::memory::Operation op) {
  auto ret = access<const quint8 *, true>(address, src, length, op);
  if (op.effectful && _tb) {
    using Write = sim::trace::WriteThroughPayload<Address>;
    api::trace::Buffer::Guard<true> guard(
        _tb, sizeof(api::packet::Packet<Write>), _device.id, Write::flags());
    if (guard) {
      auto payload = Write{.address = address, .length = length};
      auto it = new (guard.data())
          api::packet::Packet<Write>(_device.id, payload, Write::flags());
    }
  }
  return ret;
}

template <typename Address> void SimpleBus<Address>::clear(quint8 fill) {
  for (auto &region : _regions)
    region.target->clear(fill);
}

template <typename Address>
void SimpleBus<Address>::setInterposer(
    sim::api::memory::Interposer<Address> *inter) {
  this->_inter = inter;
}

template <typename Address>
void SimpleBus<Address>::dump(quint8 *dest, qsizetype maxLen) const {
  if (maxLen <= 0)
    throw std::logic_error("dump requires non-0 size");
  for (auto rit = _regions.crbegin(); rit != _regions.crend(); ++rit) {
    auto adjust = rit->span.minOffset;
    rit->target->dump(dest + adjust, maxLen - adjust);
  }
}

template <typename Address>
void SimpleBus<Address>::setTraceBuffer(api::trace::Buffer *tb) {
  _tb = tb;
  // TODO: cast memory targets to Producer and setTraceBuffer.
}

template <typename Address> void SimpleBus<Address>::trace(bool enabled) {
  if (_tb)
    _tb->trace(_device.id, enabled);
}

template <typename Address>
quint8 SimpleBus<Address>::packetSize(api::packet::Flags flags) const {
  throw std::logic_error("unimplemented");
}

template <typename Address>
bool SimpleBus<Address>::applyTrace(bits::span<const quint8> payload,
                                    api::packet::Flags flags) {
  throw std::logic_error("unimplemented");
}

template <typename Address>
bool SimpleBus<Address>::unapplyTrace(bits::span<const quint8> payload,
                                      api::packet::Flags flags) {
  throw std::logic_error("unimplemented");
}

template <typename Address>
void SimpleBus<Address>::pushFrontTarget(AddressSpan at,
                                         api::memory::Target<Address> *target) {
  _regions.push_front(Region{at, target});
}

template <typename Address>
sim::api::memory::Target<Address> *
SimpleBus<Address>::deviceAt(Address address) {
  return regionAt(address).target;
}

template <typename Address>
typename SimpleBus<Address>::Region
SimpleBus<Address>::regionAt(Address address) {
  for (auto reg : _regions) {
    if (reg.span.minOffset <= address && address <= reg.span.maxOffset)
      return reg;
  }
  return {{}, nullptr};
}
} // namespace sim::memory
