/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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
#include <QtCore>
#include <set>
#include <sim/api2/memory/target.hpp>
#include <stack>
#include <zpp_bits.h>
#include "../device.hpp"
#include "../frame.hpp"
#include "../packets.hpp"
#include "./iterator.hpp"

namespace sim::api2::trace {
class PathGuard;
class Sink;
namespace detail {
using namespace sim::api2::frame::header;
using namespace sim::api2::packet::header;
using namespace sim::api2::packet::payload;
using Fragment = std::variant<std::monostate, Trace, Extender, Clear, PureRead, ImpureRead, Write, Variable>;
} // namespace detail
using Fragment = detail::Fragment;
enum class Action { Assert, Break, Record, None };
struct Filter {
  virtual Action operator()(device::ID, quint32) = 0;
};
struct TraceFilter : public Filter {
  const device::ID target = 0;
  virtual Action operator()(device::ID dev, quint32) override {
    return (dev == target) ? Action::Record : Action::None;
  };
};
template <typename T> struct ValueFilter : public Filter {
  const sim::api2::memory::Target<T> _target;
  const quint32 _address;
  const quint8 _valueLength = 2;
  std::set<packet::VariableBytes<4>> values;
  virtual Action operator()(device::ID dev, quint32 address) override {
    static constexpr auto gs = memory::Operation{.type = memory::Operation::Type::BufferInternal, .kind = {}};
    if (_address != address) return Action::None;
    auto _value = packet::VariableBytes<4>(_valueLength);
    auto val = _target.read(address, std::span(_value.bytes.data(), _valueLength), gs);
    if (values.contains(_value)) return Action::Record;
    return Action::None;
  };
};

struct FilterEvent {
  device::ID deviceID;
  Action action;
  quint32 address;
};
template <typename Address>
  requires(std::is_signed<Address>::is_signed)
std::function<Action(device::ID, Address)> filter;
// If you inherit from this, you will likely want to inherit IteratorImpl as
// well. IteratorImpl allows polymorphic implementation of this class while
// mantaining a stable ABI for the iterator class.
// TODO: Add additional channel for command / simulation packets.
// Simulation packets are notifications such as "there's no MMIO".
// Command packets may set memory values, step forward some number of ticks.
// With these changes, the trace buffer can become single point of
// communication\ between the UI and the simulation.
class Buffer {
public:
  virtual ~Buffer() = default;

  // Must implicitly call updateFrameHeader to fix back links / lengths.
  virtual bool writeFragment(const api2::trace::Fragment &) = 0;
  template <packet::HasPath T> bool writeFragmentWithPath(T &&t) {
    t.path = currentPath();
    return writeFragment(Fragment{t});
  }

  virtual bool updateFrameHeader() = 0;

  // Remove the last frame from the buffer.
  // TODO: replace with integration for iterators / std::erase.
  virtual void dropLast() = 0;
  // Deriving classes MUST also call this implementation of clear() if overriding it.
  virtual void clear() { _paths = {paths_init()}; }

  virtual FrameIterator cbegin() const = 0;
  virtual FrameIterator cend() const = 0;
  virtual FrameIterator crbegin() const = 0;
  virtual FrameIterator crend() const = 0;
  // Paths must be stored on TB and not some other object, since the average target only has access to a TB.
  // Use a PathGuard to manipulate the current path.
  quint16 currentPath() const { return _paths.top(); }

  // Add, remove, or modify filters.
  virtual bool trace(device::ID deviceID, bool enabled = true) = 0;
  virtual quint16 addFilter(std::unique_ptr<sim::api2::trace::Filter>) = 0;
  virtual void removeFilter(quint16 id) = 0;
  virtual void replaceFilter(quint16 id, std::unique_ptr<sim::api2::trace::Filter>) = 0;

  // Process the events produced by the filters.
  virtual std::span<sim::api2::trace::FilterEvent> events() const = 0;
  virtual void clearEvents() = 0;

protected:
  void pushPath(packet::path_t path) { _paths.push(path); }
  void popPath() { _paths.pop(); }
  // stacks don't take initializer lists...
  static std::stack<packet::path_t> paths_init() {
    std::stack<packet::path_t> stack;
    stack.push(0);
    return stack;
  }
  friend class PathGuard;
  std::stack<packet::path_t> _paths = {paths_init()};
};

// Helper to enable RAII for pushing/popping paths on buffer.
class PathGuard {
public:
  PathGuard(Buffer *buffer, packet::path_t path) : _buffer(buffer), _path(path) {
    if (_buffer && _buffer->currentPath() != _path) _buffer->pushPath(_path);
  }
  ~PathGuard() {
    if (_buffer && _buffer->currentPath() == _path) _buffer->popPath();
  }
  PathGuard(const PathGuard &) = delete;
  PathGuard &operator=(const PathGuard &) = delete;
  PathGuard(PathGuard &&) = default;
  PathGuard &operator=(PathGuard &&) = default;

private:
  quint16 _path = 0;
  Buffer *_buffer = 0;
};
} // namespace sim::api2::trace
