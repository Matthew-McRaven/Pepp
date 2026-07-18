/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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
#include "core/ds/alloc/paged.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/api/trace.hpp"

// Helper class to buffer a series of characters for read/write for memory-mapped IO.
// Backed by a datastructure that can grow arbitrarily without reallocating or moving existing elements.
//
class IOQueue {
public:
  struct Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = u8;
    explicit Iterator();
    Iterator(IOQueue *q, size_t index);
    Iterator &operator++();
    Iterator operator++(int);
    // treat all _index beyond _q->max_index as equal to eachother.
    // This gives us a nice property that the end iterator can be init'ed to _index=-1, and all iterators beyond the end
    // will compare equal to it. Essentially, this lets us avoid invalidation of end iterator on insertion.
    bool operator!=(const Iterator &other) const;
    bool at_end() const;
    u8 operator*() const;
    u8 value_or(u8) const;

  private:
    IOQueue *_q;
    size_t _index = -1;
  };
  Iterator begin();
  Iterator end();
  void push(u8 value);
  u8 at(Address index) const;
  void clear();
  size_t size() const;
  bool empty() const;
  // Read the most recent value in the queue, or return default value if empty.
  u8 latest_or(u8 def) const;

private:
  Address _max_index;
  pepp::bts::PagedPool<u8> _data;
};

// This class differs from the previous memory-mapped IO implementation in the previous version of the simulator.
// The previous version formed a directed acyclic graph of values over time, and provided iterators that could walk the
// DAG. From a memory efficiency perspective, this was exceedingly inefficient due to the number of pointers stored per
// element. That design made it easy to implement step backwards within the class, but the complexity of communicating
// state changes with the IO pane remained.
// This iteration uses an unbounded queue of values, which uses an order of magnitude less memory to store the series of
// values, at the expense of stepping backwards being more complex within this class. Complexity in communicating with
// the UI should remain unchanged.
// I opted to merge input & output into a single class, since real memory-mapped registers can be bi-directional.
// When the class is input only, the output queue is unused. When the class is output only, the input queue is unused.
// For bi-directional registers, input queue is used on read, and output queue is used on write.
class MemoryMappedRegister final : public Device, public Target, public Traceable {
public:
  static const inline std::string compatible = "io,reg";
  enum class IODirection : u8 {
    None = 0,
    Input = 1 << 0,
    Output = 1 << 1,
  };
  struct Configuration : public Device::Configuration {
    u8 fill{0};
    MemoryMappedRegister::IODirection direction = MemoryMappedRegister::IODirection::None;
    AddressSpan span{};
  };
  MemoryMappedRegister(Configuration device);
  ~MemoryMappedRegister() = default;
  MemoryMappedRegister(MemoryMappedRegister &&other) noexcept = default;
  MemoryMappedRegister &operator=(MemoryMappedRegister &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  MemoryMappedRegister(const MemoryMappedRegister &) = delete;
  MemoryMappedRegister &operator=(const MemoryMappedRegister &) = delete;
  std::span<const u8> data() const;

  // Device interface
  const Device::Configuration &config() const override;
  const Device::ID id() const override;
  Device::Type type() const override;
  u64 features() const override;
  std::unique_ptr<DeviceSerializer> serializer() const override;
  static std::unique_ptr<DeviceSerializer> make_serializer();

  // TraceSource interface
  void set_buffer(Buffer *tb) override;
  const Buffer *buffer() const override;
  bool can_generate_traces() const override;
  void trace(bool enabled) override;
  bool traced() const override;

  // Target interface
  AddressSpan span() const override;
  // If input, consume value from input queue. If output, return most recent value from output queue.
  Result read(Address address, bits::span<u8> dest, Operation op) const override;
  // If output, append to output queue. Noop otherwise.
  Result write(Address address, bits::span<const u8> src, Operation op) override;
  void clear(u8 fill) override;
  // If output, return most recent value written. Otherwise, return the current value of input.
  void dump(bits::span<u8> dest) const override;

private:
  Configuration _config;
  IOQueue _input, _output;
  mutable IOQueue::Iterator _input_it;
  Buffer *_tb = nullptr;
};

consteval void is_bitflags(MemoryMappedRegister::IODirection);
