/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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
#include "sim3/api/memory_address.hpp"
#include "sim3/api/traced/memory_target.hpp"
#include "sim3/api/traced/trace_endpoint.hpp"
#include "sim3/trace/packet_utils.hpp"
#include "utils/bits/copy.hpp"

namespace sim::memory {
template <typename Address>
class Dense : public api2::memory::Target<Address>, public api2::trace::Source, public api2::trace::Sink {
public:
  using AddressSpan = typename api2::memory::AddressSpan<Address>;
  Dense(api2::device::Descriptor device, AddressSpan span, quint8 defaultValue = 0);
  ~Dense() = default;
  Dense(Dense &&other) noexcept = default;
  Dense &operator=(Dense &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  Dense(const Dense &) = delete;
  Dense &operator=(const Dense &) = delete;

  // API v2
  // Target interface
  sim::api2::device::ID deviceID() const override { return _device.id; }
  sim::api2::device::Descriptor device() const override { return _device; }
  AddressSpan span() const override;
  api2::memory::Result read(Address address, bits::span<quint8> dest, api2::memory::Operation op) const override;
  api2::memory::Result write(Address address, bits::span<const quint8> src, api2::memory::Operation op) override;
  void clear(quint8 fill) override;
  void dump(bits::span<quint8> dest) const override;

  // Sink interface
  bool analyze(const api2::trace::PacketIterator iter, api2::trace::Direction) override;

  // Source interface
  void setBuffer(api2::trace::Buffer *tb) override;
  const api2::trace::Buffer *buffer() const override { return _tb; }
  void trace(bool enabled) override;

  // Helpers
  const quint8 *constData() const;
  // Enables re-use of memory across multiple runs.
  void setDevice(api2::device::Descriptor device) { _device = device; }
  void setSpan(AddressSpan span) {
    _span = span;
    // Ensure no data leaks between configurations.
    _data.resize(size_inclusive(_span));
    _data.fill(_fill);
  }

private:
  quint8 _fill;
  AddressSpan _span;
  api2::device::Descriptor _device;
  QVector<quint8> _data;
  api2::trace::Buffer *_tb = nullptr;
};

template <typename Address>
sim::memory::Dense<Address>::Dense(api2::device::Descriptor device, AddressSpan span, quint8 fill)
    : _fill(fill), _span(span), _device(device) {
  _data.fill(_fill, size_inclusive(_span)); // Resizes before filling.
}

template <typename Address>
typename sim::memory::Dense<Address>::AddressSpan sim::memory::Dense<Address>::span() const {
  return _span;
}

template <typename Address> void sim::memory::Dense<Address>::clear(quint8 fill) {
  this->_fill = fill;
  this->_data.fill(this->_fill);
}

template <typename Address> void Dense<Address>::dump(bits::span<quint8> dest) const {
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  bits::memcpy(dest, bits::span<const quint8>{_data.constData(), std::size_t(_data.size())});
}

template <typename Address> void Dense<Address>::trace(bool enabled) {
  if (this->_tb) _tb->trace(_device.id, enabled);
}

template <typename Address> const quint8 *sim::memory::Dense<Address>::constData() const { return _data.constData(); }

namespace detail {
template <typename Address> struct PayloadHelper {
  PayloadHelper(Address address, Dense<Address> *dense) : address(address), dense(dense) {}

  static constexpr auto op = api2::memory::Operation{
      .type = api2::memory::Operation::Type::BufferInternal,
      .kind = api2::memory::Operation::Kind::data,
  };

  Address operator()(const api2::packet::payload::Variable &frag) {
    std::array<quint8, api2::packet::payload::Variable::N> tmp;
    tmp.fill(0);

    Address len = std::min<Address>(tmp.size(), frag.payload.len);
    auto span = bits::span<quint8>{tmp.data(), len};

    // Get current value and XOR with XOR-encoded bytes, which we write back.
    dense->read(address, span, op);
    bits::memcpy_xor(span, span, bits::span<const quint8>{frag.payload.bytes.data(), frag.payload.len});
    dense->write(address, span, op);
    return len;
  }

  // Will need to implement if we create other payload fragments.
  Address operator()(const auto &frag) const { throw std::logic_error("unimplemented"); }

  Address address;
  Dense<Address> *dense;
};
} // namespace detail

template <typename Address>
bool Dense<Address>::analyze(api2::trace::PacketIterator iter, api2::trace::Direction direction) {
  auto header = *iter;
  if (!std::visit(sim::trace2::IsSameDevice{_device.id}, header)) return false;
  // Read has no side effects, dense only issues pure reads.
  // Therefore we only need to handle out write packets.
  else if (std::holds_alternative<api2::packet::header::Write>(header)) {
    auto hdr = std::get<api2::packet::header::Write>(header);
    Address address = hdr.address.to_address<Address>();
    // forward vs backwards does not matter for dense memory,
    // since payloads are XOR encoded. We can compute (current XOR payload)
    // to determine the updated memory values.
    for (auto payload : iter) address += std::visit(detail::PayloadHelper<Address>(address, this), payload);
  }
  return true;
}

template <typename Address> void Dense<Address>::setBuffer(api2::trace::Buffer *tb) { _tb = tb; }

template <typename Address>
api2::memory::Result Dense<Address>::read(Address address, bits::span<quint8> dest, api2::memory::Operation op) const {
  using E = api2::memory::Error;
  using Operation = sim::api2::memory::Operation;
  // Length is 1-indexed, address are 0, so must offset by -1.
  auto maxDestAddr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < _span.lower() || maxDestAddr > _span.upper()) throw E(E::Type::OOBAccess, address);
  auto offset = address - _span.lower();
  auto src = bits::span<const quint8>{_data.data(), std::size_t(_data.size())}.subspan(offset);
  // Ignore reads from UI, since this device only issues pure reads.
  // Ignore reads from buffer internal operations.
  if (!(op.type == Operation::Type::Application || op.type == Operation::Type::BufferInternal) && _tb)
    _tb->emitPureRead<Address>(_device.id, offset, src.size());
  bits::memcpy(dest, src);
  return {};
}

template <typename Address>
api2::memory::Result Dense<Address>::write(Address address, bits::span<const quint8> src, api2::memory::Operation op) {
  using E = api2::memory::Error;
  using Operation = sim::api2::memory::Operation;
  // Length is 1-indexed, address are 0, so must offset by -1.
  auto maxDestAddr = (address + std::max<Address>(0, src.size() - 1));
  if (address < _span.lower() || maxDestAddr > _span.upper()) throw E(E::Type::OOBAccess, address);
  auto offset = address - _span.lower();
  auto dest = bits::span<quint8>{_data.data(), std::size_t(_data.size())}.subspan(offset);
  // Record changes, even if the come from UI. Otherwise, step back fails.
  if (op.type != Operation::Type::BufferInternal && _tb) _tb->emitWrite<Address>(_device.id, offset, src, dest);
  bits::memcpy(dest, src);
  return {};
}

} // namespace sim::memory
