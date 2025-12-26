/*
 * /Copyright (c) 2024-2025. Stanley Warford, Matthew McRaven
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
#include <QtCore>
#include <zpp_bits.h>
#include "./trace_frame.hpp"
#include "trace_packets.hpp"

namespace sim::api2::trace {
enum class Level {
  Frame = 1,
  Packet = 2,
  Payload = 3,
};
inline auto operator<=>(Level lhs, Level rhs) { return ((int)lhs) <=> ((int)rhs); }

struct IteratorImpl {
  virtual std::size_t end() const = 0;
  virtual std::size_t size_at(std::size_t loc, Level level) const = 0;
  virtual Level at(std::size_t loc) const = 0;
  virtual frame::Header frame(std::size_t loc) const = 0;
  virtual sim::api2::packet::Header packet(std::size_t loc) const = 0;
  virtual sim::api2::packet::Payload payload(std::size_t loc) const = 0;
  virtual std::size_t next(std::size_t loc, Level level) const = 0;
  virtual std::size_t prev(std::size_t loc, Level level) const = 0;
};

enum class Direction {
  Forward,
  Reverse,
};
// "base class" for iterators.
// Specialization of this class with <Arg> and <Arg, ...args>
// will enable my heirarchical iterators.
template <Level... Args> struct HierarchicalIterator;

// Prevent instantiation of iterator for Iterator<Level::Payload>
template <Level Current> struct HierarchicalIterator<Current> {
public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = quint64;
  using _helper = typename std::conditional<Current == Level::Packet, packet::Header, packet::Payload>::type;
  using value_type = typename std::conditional<Current == Level::Frame, frame::Header, _helper>::type;
  using pointer = const value_type *;
  using reference = value_type &;

  HierarchicalIterator(const IteratorImpl *impl, std::size_t location, Direction dir = Direction::Forward)
      : _impl(impl), _location(location), _dir(dir) {}

  HierarchicalIterator &operator++() {
    if (_dir == Direction::Forward) _location = _impl->next(_location, Current);
    else _location = _impl->prev(_location, Current);
    return *this;
  }

  HierarchicalIterator &operator--() {
    if (_dir == Direction::Forward) _location = _impl->prev(_location, Current);
    else _location = _impl->next(_location, Current);
    return *this;
  }

  HierarchicalIterator &operator++(int) {
    auto ret = *this;
    ++(*this);
    return ret;
  }

  HierarchicalIterator &operator--(int) {
    auto ret = *this;
    --(*this);
    return ret;
  }

  bool operator==(HierarchicalIterator other) const {
    return _impl == other._impl && _location == other._location && _dir == other._dir;
  }

  bool operator!=(HierarchicalIterator other) const { return !(other == *this); }

  std::size_t fragment_size() const { return _impl->size_at(_location, Current); }

  value_type operator*() const {
    if constexpr (Current == Level::Frame) return _impl->frame(_location);
    else if constexpr (Current == Level::Packet) return _impl->packet(_location);
    else return _impl->payload(_location);
  }

protected:
  const IteratorImpl *_impl;
  std::size_t _location = 0;
  Direction _dir;
};

// Defer to above implementation in all cases except those handling iteration.
// If created as a forward iterator, cbegin/cend will create forward iterators.
// If created as a reverse iterator, cbegin/cend will create reverse iterators.
// This should allow analyzers to be agnostic to the direction of iteration.
template <Level Current, Level... Descendants>
struct HierarchicalIterator<Current, Descendants...> : public HierarchicalIterator<Current> {
public:
  HierarchicalIterator(const IteratorImpl *impl, std::size_t location, Direction dir = Direction::Forward)
      : HierarchicalIterator<Current>(impl, location, dir) {}

  // Defer to internal implementations to avoid duplication of complex iteration
  // code.

  HierarchicalIterator<Descendants...> cbegin() const {
    return this->_dir == Direction::Forward ? this->_cbegin() : this->_crbegin();
  }

  HierarchicalIterator<Descendants...> cend() const {
    return this->_dir == Direction::Forward ? this->_cend() : this->_crend();
  }

  HierarchicalIterator<Descendants...> crbegin() const {
    return this->_dir == Direction::Forward ? this->_crbegin() : this->_cbegin();
  }

  HierarchicalIterator<Descendants...> crend() const {
    return this->_dir == Direction::Forward ? this->_crend() : this->_cend();
  }

protected:
  HierarchicalIterator<Descendants...> _cbegin() const {
    // Skip current element, because this returns a iterator for children.
    auto to_next = this->_impl->size_at(this->_location, Current);
    return HierarchicalIterator<Descendants...>(this->_impl, this->_location + to_next);
  }

  HierarchicalIterator<Descendants...> _cend() const {
    // Skip current element, because this returns a iterator for children.
    auto to_next = this->_impl->size_at(this->_location, Current);
    std::size_t next = this->_location + to_next;
    if (next != this->_impl->end()) {
      auto next_type = this->_impl->at(next);
      // If the next item is below our level of abstraction,
      // then we need to search for the next item that is at our level of
      // abstraction.
      if ((int)Current < (int)next_type) next = this->_impl->next(next, Current);
    }
    return HierarchicalIterator<Descendants...>(this->_impl, next);
  }

  HierarchicalIterator<Descendants...> _crbegin() const {
    // Figure out what next level is.
    Level below;
    switch (Current) {
    case Level::Frame: below = Level::Packet; break;
    case Level::Packet: below = Level::Payload; break;
    default:
      static const char *const e = "Unreachable?";
      qCritical(e);
      throw std::logic_error(e);
    }

    // Find the first successor element at the current level of abstraction,
    // and from the successor, find the previous element at
    // the next lower level of asbtraction.
    std::size_t loc = this->_location;
    auto next_above = this->_impl->next(this->_location, Current);

    // If next(...) hits end, prev will "do the right thing".
    // Our only risk is that prev hits the end sentinel.
    loc = this->_impl->prev(next_above, below);
    return HierarchicalIterator<Descendants...>(this->_impl, loc, Direction::Reverse);
  }

  HierarchicalIterator<Descendants...> _crend() const {
    // Current fragment must be at higher level of abstraction than descendant
    // fragments. Therefore, the location of this fragment makes for a good
    // "past the end" iterator value.
    return HierarchicalIterator<Descendants...>(this->_impl, this->_location, Direction::Reverse);
  }
};
using FrameIterator = HierarchicalIterator<Level::Frame, Level::Packet, Level::Payload>;
using PacketIterator = HierarchicalIterator<Level::Packet, Level::Payload>;
// Needed to enable range-based for loops
template <Level... args> auto begin(HierarchicalIterator<args...> &iter) { return iter.cbegin(); }
template <Level... args> auto end(HierarchicalIterator<args...> &iter) { return iter.cend(); }

} // namespace sim::api2::trace
