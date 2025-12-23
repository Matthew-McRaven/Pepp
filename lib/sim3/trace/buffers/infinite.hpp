/*
 * /Copyright (c) 2023-2025. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "sim3/api/traced/trace_buffer.hpp"
#include "../packet_utils.hpp"
#include "lru/cache.hpp"

namespace sim::trace2 {
class IsTraced {
public:
  IsTraced(QSet<sim::api2::device::ID> devices) : _devices(devices){};
  template <HasDevice Header> bool operator()(const Header &header) const {
    quint16 val = header.device;
    return _devices.contains(val);
  };
  bool operator()(const auto &header) const { return false; }

private:
  QSet<sim::api2::device::ID> _devices;
};

template <typename Target> struct IsType {
  bool operator()(const auto &header) { return std::same_as<Target, std::remove_cv<decltype(header)>>; }
};

class InfiniteBuffer : public api2::trace::Buffer, public api2::trace::IteratorImpl {
public:
  using FrameIterator = api2::trace::FrameIterator;
  InfiniteBuffer();
  // Buffer interface
  bool trace(quint16 deviceID, bool enabled) override;
  bool traced(quint16 deviceID) const override;
  bool writeFragment(const api2::trace::Fragment &) override;
  bool updateFrameHeader() override;
  void dropLast() override;
  void clear() override;
  FrameIterator cbegin() const override;
  FrameIterator cend() const override;
  FrameIterator crbegin() const override;
  FrameIterator crend() const override;

private:
  QSet<sim::api2::device::ID> _sinks = {};
  std::size_t _lastFrameStart = 0;
  // Need to be mutable so that IteratorImpl can read from them.
  mutable std::vector<std::byte> _data = {};

  mutable LRU::Cache<std::size_t, std::size_t> _backlinks;
  zpp::bits::in<decltype(_data)> _in;
  zpp::bits::out<decltype(_data)> _out;
  // Like normal next, except you can control if the iterator jumps between frames
  // or walks cell-by-cell. user-facing next calls can take advantage of the speed,
  // while internal next calls can use next to fill backlink cache.
  std::size_t next(std::size_t loc, api2::trace::Level level, bool allow_jumps) const;
  std::size_t last_before(std::size_t start, std::size_t end, api2::trace::Level payload) const;

  // IteratorImpl interface
public:
  std::size_t end() const override;
  std::size_t size_at(std::size_t loc, api2::trace::Level level) const override;
  api2::trace::Level at(std::size_t loc) const override;
  api2::frame::Header frame(std::size_t loc) const override;
  api2::packet::Header packet(std::size_t loc) const override;
  api2::packet::Payload payload(std::size_t loc) const override;
  std::size_t next(std::size_t loc, api2::trace::Level level) const override;
  std::size_t prev(std::size_t loc, api2::trace::Level level) const override;
};
} // namespace sim::trace2
