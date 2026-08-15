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
#include "core/sim/debugger/trace_recorder.hpp"


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
class FIFORegister final : public Device, public Target, public Traceable {
public:
  // Helper class to buffer a series of characters for read/write for memory-mapped IO.
  // Backed by a datastructure that can grow arbitrarily without reallocating or moving existing elements.
  //
  class FIFO {
  public:
    struct Iterator {
    public:
      using iterator_category = std::forward_iterator_tag;
      using difference_type = std::ptrdiff_t;
      using value_type = u8;
      explicit Iterator();
      Iterator(FIFO *q, size_t index);
      Iterator &operator++();
      Iterator operator++(int);
      Iterator &operator--();
      Iterator operator--(int);
      bool operator!=(const Iterator &other) const;
      bool operator==(const Iterator &other) const;
      bool at_end() const;
      u8 operator*() const;
      u8 value_or(u8) const;

    private:
      FIFO *_q;
      size_t _index = -1;
    };
    Iterator begin();
    // On insert, previously captured end() iterator will become invalidated.
    Iterator end();
    void push(u8 value);
    u8 pop_back();
    u8 at(Address index) const;
    void clear();
    size_t size() const noexcept;
    bool empty() const noexcept;
    // Read the most recent value in the queue, or return default value if empty.
    u8 latest_or(u8 def) const noexcept;

  private:
    Address _max_index = 0;
    pepp::bts::PagedPool<u8> _data;
  };

  static const inline std::string compatible = "io,fifo";
  enum class Direction : u8 {
    // "none"
    None = 0,
    // "in"
    Input = 1 << 0,
    // "out"
    Output = 1 << 1,
    // Combination is "inout"
  };
  struct Configuration : public Device::Configuration {
    u8 fill{0};
    FIFORegister::Direction direction = FIFORegister::Direction::None;
    AddressSpan span{};
    // Options are "yield_default" and "raise_error"
    FailPolicy fail_policy = FailPolicy::RaiseError;
  };
  FIFORegister(Configuration device);
  ~FIFORegister() = default;
  FIFORegister(FIFORegister &&other) noexcept = default;
  FIFORegister &operator=(FIFORegister &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  FIFORegister(const FIFORegister &) = delete;
  FIFORegister &operator=(const FIFORegister &) = delete;

  FIFO &input();
  // Call FIFO::pop_back() on _input_it. Saturates at the front yielding the fill value rather than wrapping.
  u8 rewind_input();
  FIFO &output();

  // Device interface
  const Device::Configuration &config() const override;
  const Configuration &casted_config() const;
  const Device::ID id() const override;
  Device::Type type() const override;
  u64 features() const override;
  std::unique_ptr<DeviceSerializer> serializer() const override;
  static std::unique_ptr<DeviceSerializer> make_serializer();

  // TraceSource interface
  void set_recorder(const trace::Recorder &recorder) override;
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
  FIFO _input, _output;
  mutable FIFO::Iterator _input_it;
  mutable trace::Recorder _trace;
};

consteval void is_bitflags(FIFORegister::Direction);
