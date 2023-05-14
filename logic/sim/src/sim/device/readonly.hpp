#pragma once
#include "sim/api.hpp"

namespace sim::memory {
template <typename Address>
class ReadOnly : public api::memory::Target<Address>,
                 api::memory::Initiator<Address> {
public:
  using AddressSpan = typename api::memory::Target<Address>::AddressSpan;
  ReadOnly(bool hardFail);
  ~ReadOnly() = default;
  ReadOnly(ReadOnly &&other) noexcept = default;
  ReadOnly &operator=(ReadOnly &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  ReadOnly(const ReadOnly &) = delete;
  ReadOnly &operator=(const ReadOnly &) = delete;

  // Target interface
  AddressSpan span() const override;
  api::memory::Result read(Address address, bits::span<quint8> dest,
                           api::memory::Operation op) const override;
  api::memory::Result write(Address address, bits::span<const quint8> src,
                            api::memory::Operation op) override;
  void clear(quint8 fill) override;
  void setInterposer(api::memory::Interposer<Address> *inter) override;
  void dump(bits::span<quint8> dest) const override;

  // Initiator interface
  void setTarget(sim::api::memory::Target<Address> *target) override;
  void setTarget(void *port,
                 sim::api::memory::Target<Address> *target) override;

private:
  bool _hardFail;
  sim::api::memory::Target<Address> *_target = nullptr;
};

template <typename Address>
ReadOnly<Address>::ReadOnly(bool hardFail) : _hardFail(hardFail) {}

template <typename Address>
typename ReadOnly<Address>::AddressSpan ReadOnly<Address>::span() const {
  return _target->span();
}

template <typename Address>
api::memory::Result ReadOnly<Address>::read(Address address,
                                            bits::span<quint8> dest,
                                            api::memory::Operation op) const {
  return _target->read(address, dest, op);
}

template <typename Address>
api::memory::Result ReadOnly<Address>::write(Address address,
                                             bits::span<const quint8> src,
                                             api::memory::Operation op) {
  // Length is 1-indexed, address are 0, so must convert by -1.
  auto maxDestAddr = (address + std::max<Address>(0, src.size() - 1));
  if (address < _target->span().minOffset ||
      maxDestAddr > _target->span().maxOffset)
    return {.completed = false,
            .pause = true,
            .error = api::memory::Error::OOBAccess};
  else if (!op.effectful) {
    return _target->write(address, src, op);
  } else if (_hardFail) {
    return {.completed = false,
            .pause = false,
            .error = sim::api::memory::Error::writeToRO};
  } else {
    return {.completed = true,
            .pause = false,
            .error = sim::api::memory::Error::Success};
  }
}

template <typename Address> void ReadOnly<Address>::clear(quint8 fill) {
  _target->clear(fill);
}

template <typename Address>
void ReadOnly<Address>::setInterposer(api::memory::Interposer<Address> *inter) {
  _target->setInterposer(inter);
}

template <typename Address>
void ReadOnly<Address>::dump(bits::span<quint8> dest) const {
  _target->dump(dest);
}

template <typename Address>
void ReadOnly<Address>::setTarget(sim::api::memory::Target<Address> *target) {
  _target = target;
}

template <typename Address>
void ReadOnly<Address>::setTarget(void *port,
                                  sim::api::memory::Target<Address> *target) {
  throw std::logic_error("Unimplemented");
}

} // namespace sim::memory
