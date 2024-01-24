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
#include "bits/operations/copy.hpp"
#include "sim/api2.hpp"
#include "sim/trace/common.hpp"
#include "sim/trace2/packet_utils.hpp"

namespace sim::memory {
template <typename Address>
class Dense : public api2::memory::Target<Address>,
              public api2::trace::Source,
              public api2::trace::Sink {
public:
  using AddressSpan = typename api2::memory::AddressSpan<Address>;
  Dense(api2::device::Descriptor device, AddressSpan span,
        quint8 defaultValue = 0);
  ~Dense() = default;
  Dense(Dense &&other) noexcept = default;
  Dense &operator=(Dense &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  Dense(const Dense &) = delete;
  Dense &operator=(const Dense &) = delete;

  // API v2
  // Target interface
  api2::memory::Result read(Address address, bits::span<quint8> dest, api2::memory::Operation op) const override;
  api2::memory::Result write(Address address, bits::span<const quint8> src, api2::memory::Operation op) override;
  AddressSpan span() const override;
  void clear(quint8 fill) override;
  void dump(bits::span<quint8> dest) const override;

  // Sink interface
  bool filter(const api2::packet::Header& header) override;
  bool analyze(const api2::packet::Header& header, const std::span<api2::packet::Payload> &, Direction) override;
  // Source interface
  void setBuffer(api2::trace::Buffer *tb) override;
  void trace(bool enabled) override;

  // Helpers
  const quint8 *constData() const;

private:
  quint8 _fill;
  AddressSpan _span;
  api2::device::Descriptor _device;
  QVector<quint8> _data;
  api2::trace::Buffer *_tb = nullptr;

};

template <typename Address>
sim::memory::Dense<Address>::Dense(api2::device::Descriptor device,
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

template <typename Address> void Dense<Address>::trace(bool enabled) {
  if (this->_tb) _tb->trace(_device.id, enabled);
}

template <typename Address>
const quint8 *sim::memory::Dense<Address>::constData() const {
  return _data.constData();
}

template<typename Address>
bool Dense<Address>::filter(const api2::packet::Header &header)
{
  return std::visit(sim::trace2::IsSameDevice{_device.id}, header);
}

template<typename Address>
bool Dense<Address>::analyze(const api2::packet::Header &header, const std::span<api2::packet::Payload> &, Direction)
{
  throw std::logic_error("unimplemented");
  return true;
}

template<typename Address>
void Dense<Address>::setBuffer(api2::trace::Buffer *tb)
{
  _tb = tb;
}

template<typename Address>
api2::memory::Result Dense<Address>::read(Address address, bits::span<quint8> dest, api2::memory::Operation op) const
{
  using E = api2::memory::Error<Address>;
  using Operation = sim::api2::memory::Operation;
  // Length is 1-indexed, address are 0, so must offset by -1.
  auto maxDestAddr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < _span.minOffset || maxDestAddr > _span.maxOffset)
    throw E(E::Type::OOBAccess, address);
  auto offset = address - _span.minOffset;
  auto src = bits::span<const quint8>{_data.data(), std::size_t(_data.size())}.subspan(offset);
  // Don't need to record reads from UI, since they can cause no side-effects.
  if (op.type != Operation::Type::Application && _tb) sim::trace2::emitPureRead<Address>(_tb, _device.id, offset, src.size());
  bits::memcpy(dest, src);
  return {};
}

template<typename Address>
api2::memory::Result Dense<Address>::write(Address address, bits::span<const quint8> src, api2::memory::Operation op)
{
  using E = api2::memory::Error<Address>;
  // Length is 1-indexed, address are 0, so must offset by -1.
  auto maxDestAddr = (address + std::max<Address>(0, src.size() - 1));
  if (address < _span.minOffset || maxDestAddr > _span.maxOffset)
    throw E(E::Type::OOBAccess, address);
  auto offset = address - _span.minOffset;
  auto dest = bits::span<quint8>{_data.data(), std::size_t(_data.size())}.subspan(offset);
  // Always record changes, even if the come from UI. Otherwise, step back fails.
  if (_tb) sim::trace2::emitWrite<Address>(_tb, _device.id, offset, src, dest);
  bits::memcpy(dest, src);
  return {};
}


} // namespace sim::memory
