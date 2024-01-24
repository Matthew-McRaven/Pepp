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
#include "sim/api.hpp"
#include "sim/api2.hpp"

namespace sim::memory {
template <typename Address>
class ReadOnly : virtual public api::memory::Target<Address>,
                 public api::memory::Initiator<Address>,
                 public api2::memory::Target<Address>,
                 public api2::memory::Initiator<Address> {
public:
  using AddressSpan = typename api::memory::AddressSpan<Address>;
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
  void dump(bits::span<quint8> dest) const override;

  // Initiator interface
  void setTarget(sim::api::memory::Target<Address> *target) override;
  void setTarget(void *port,
                 sim::api::memory::Target<Address> *target) override;

private:
  bool _hardFail;
  sim::api::memory::Target<Address> *_target = nullptr;
  sim::api2::memory::Target<Address> *_target2 = nullptr;
  // API v2
public:
  // Target interface
  api2::memory::Result read(Address address, bits::span<quint8> dest, api2::memory::Operation op) const override;
  api2::memory::Result write(Address address, bits::span<const quint8> src, api2::memory::Operation op) override;
  // Initiator interface
  void setTarget(sim::api2::memory::Target<Address> *target, void *port) override;

};

template <typename Address>
ReadOnly<Address>::ReadOnly(bool hardFail) : _hardFail(hardFail) {}

template <typename Address>
typename ReadOnly<Address>::AddressSpan ReadOnly<Address>::span() const {
  if(_target2) return _target2->span();
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
    if(_target2) _target2->clear(fill);
    else _target->clear(fill);
}

template <typename Address>
void ReadOnly<Address>::dump(bits::span<quint8> dest) const {
    if(_target2) _target2->dump(dest);
    else _target->dump(dest);
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

template<typename Address>
api2::memory::Result ReadOnly<Address>::read(Address address, bits::span<quint8> dest, api2::memory::Operation op) const
{
    return _target2->read(address, dest, op);
}

template<typename Address>
api2::memory::Result ReadOnly<Address>::write(Address address, bits::span<const quint8> src, api2::memory::Operation op)
{
    using E = api2::memory::Error<Address>;
    // Length is 1-indexed, address are 0, so must offset by -1.
    auto maxDestAddr = (address + std::max<Address>(0, src.size() - 1));
    // Duplicate bounds-checking from target, because we can't check bounds
    // in the target without doing an access.
    if (address < _target2->span().minOffset ||
        maxDestAddr > _target2->span().maxOffset)
      throw E(E::Type::OOBAccess, address);
    // If the write is coming from the app (e.g., memory editor) allow it.
    else if(op.type == api2::memory::Operation::Type::Application)
        return _target2->write(address, src, op);
    else if(_hardFail)
        throw E(E::Type::WriteToRO, address);
    return {};
}

template<typename Address>
void ReadOnly<Address>::setTarget(sim::api2::memory::Target<Address> *target, void *port)
{
  _target2 = target;
}

} // namespace sim::memory
