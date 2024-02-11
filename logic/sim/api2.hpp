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
#include <zpp_bits.h>

#include "bits/operations/copy.hpp"
#include "bits/span.hpp"

namespace sim::api2 {

static const int VERSION = 2;

namespace device {
using ID = quint16; // Only use 9 bits (max of 512)!
struct Descriptor {
  device::ID id; // Must uniquely identify a device within a system.
  void *compatible;
  QString baseName, fullName;
};
using IDGenerator = std::function<ID()>;
} // namespace device

// A fragment is one of the data classes in packet::header, packet::payload, or
// frame::header. A packet is a one packet::header fragment followed by 0 or
// more packet::payload fragments A frame is a frame::header fragment followed
// by 0 or more packets.
namespace packet {
// Stack-allocated variable length array of bytes. When serialized, it will only
// occupy the number of bytes specified in data_len, not the size as specified
// by N. One use for this class is to serialize addresses, which may be 1-8
// bytes long. These variable-length addresses allow us to use the same packet
// for all memory accesses. For example, in Pep/10 a register is a 1 byte
// address, where main memory is 2. Our old trace system needed a packet type
// for each address size. The variable-length byte-array allows this to be
// expressed in a single packet type.
template <size_t N> struct VariableBytes {
  VariableBytes(quint8 len, bool continues = false) {
    this->len = (len & len_mask()) | (continues ? 0x80 : 0x00);
    bytes.fill(0);
  }

  VariableBytes(quint8 len, bits::span<const quint8> src, bool continues = false) {
    this->len = (len & len_mask()) | (continues ? 0x80 : 0x00);
    bits::memcpy(bits::span<quint8>{bytes.data(), len}, src);
  }

  template <std::unsigned_integral Address> static VariableBytes from_address(Address address) {
    auto len = sizeof(address);
    // Copy address bytes into bytes array.
    return VariableBytes(len, bits::span<const quint8>((quint8 *)&address, len));
  }

  template <std::unsigned_integral Address> Address to_address() const {
    Address address = 0;
    auto addr_span = bits::span<quint8>((quint8 *)&address, sizeof(address));
    // Rely on memcpy to perform bounds checking between len and
    // sizeof(address).
    bits::memcpy(addr_span, bits::span<const quint8>{bytes.data(), len});
    return address;
  }

  constexpr static zpp::bits::errc serialize(auto &archive, auto &self) {
    // Serialize array manually to avoid allocating extra 0's in the bit stream.
    // Must also de-serialize manually, otherwise archive will advance the
    // position by the allocated size of the array, not the "used" size.
    if (archive.kind() == zpp::bits::kind::out) {
      // Mask out flag bits before checking max size.
      auto len = self.len & len_mask();
      if (len > N)
        return zpp::bits::errc(std::errc::value_too_large);
      // Write out length + flags.
      zpp::bits::errc errc = archive(self.len);
      if (errc.code != std::errc())
        return errc;
      else if (self.len == 0)
        return errc;

      // Let compiler deduce [const quint8] vs [quint8].
      auto span = std::span(self.bytes.data(), len);
      return archive(zpp::bits::bytes(span, len));
    }
    // Only allow reading into nonconst objects
    else if (archive.kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      zpp::bits::errc errc = archive(self.len);
      auto len = self.len & len_mask();
      if (errc.code != std::errc())
        return errc;
      // Ignore flag bits in bounds check
      else if (len > N)
        return zpp::bits::errc(std::errc::value_too_large);
      else if (len == 0)
        return errc;

      // We serialized the length ourselves. If we pass array_view directly,
      // size will be serialzed again.
      auto array_view = bits::span<quint8>(self.bytes.data(), len);
      return archive(zpp::bits::bytes(array_view, array_view.size_bytes()));
    } else if (archive.kind() == zpp::bits::kind::in)
      throw std::logic_error("Can't read into const");
    throw std::logic_error("Unimplemented");
  }

  bool continues() const { return len & ~len_mask(); }

  static constexpr quint8 len_mask() { return 0x7f; }

  // Ensure that we don't clobber flags with a too-large array.
  static_assert(N < len_mask() + 1);

  // High order bit is used for "continues" flag.
  quint8 len = 0;
  std::array<quint8, N> bytes = {0};
};

using device_id_t = zpp::bits::varint<quint16>;
enum class DeltaEncoding : quint8 {
  XOR, // The old and new values are XOR'ed together
};

namespace header {
struct Clear {
  device_id_t device = 0;
  static constexpr std::size_t N = 8;
  VariableBytes<N> value = {0};
};

struct PureRead {
  device_id_t device = 0;
  zpp::bits::varint<quint64> payload_len = 0;
  static constexpr std::size_t N = 8;
  VariableBytes<N> address = {0};
};

// MUST be followed by 1+ payloads.
struct ImpureRead {
  device_id_t device = 0;
  static constexpr std::size_t N = 8;
  VariableBytes<N> address = {0};
};

// MUST be followed by 1+ payload.
struct Write {
  device_id_t device = 0;
  static constexpr std::size_t N = 8;
  VariableBytes<N> address = {0};
};
} // namespace header
using Header = std::variant<header::Clear, header::PureRead, header::ImpureRead, header::Write>;

namespace payload {
// Successive payloads belong to the same packet.
struct Variable {
  static constexpr std::size_t N = 32;
  VariableBytes<N> payload = {0};
};
} // namespace payload
using Payload = std::variant<payload::Variable>;
using Fragment = std::variant<Header, Payload>;
} // namespace packet

namespace frame {
// DO NOT SET "length" OR "back_offset"! The trace buffer will fill in these
// fields in with the correct offsets.
// There may be different kinds of frames.
// The most common kind is a Trace frame, which records modifications to the
// simulation. Other headers may be used to convey configuration information,
// etc.
namespace header {
struct Trace {
  // Offset (in bytes) to the start of the next header.
  // The code responsible for starting a new frame needs to
  // go back and update this field.
  // If 0, then this is the last frame in the trace.
  quint16 length = 0;
  // Number of bytes to the start of the previous FrameHeader.
  // If 0, then this is the first frame in the trace.
  zpp::bits::varint<quint16> back_offset = 0;
};
// If a single frame grows too large, its length will overflow a 16b int.
// To avoid this, the trace buffer can automatically insert an Extender header.
// Physically, it starts a new frame. Logically, the packets in each should be
// considered to belong to the same frame.
struct Extender {
  quint16 length = 0, back_offset = 0xFFFF;
};
} // namespace header
using Header = std::variant<header::Trace, header::Extender>;
} // namespace frame

namespace trace {
enum class Mode {
  Realtime, // Trace frames are parsed as they are received
  Deferred, // Trace frames will be parsed at some later point.
};

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
  virtual packet::Header packet(std::size_t loc) const = 0;
  virtual packet::Payload payload(std::size_t loc) const = 0;
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
    if (_dir == Direction::Forward)
      _location = _impl->next(_location, Current);
    else
      _location = _impl->prev(_location, Current);
    return *this;
  }

  HierarchicalIterator &operator--() {
    if (_dir == Direction::Forward)
      _location = _impl->prev(_location, Current);
    else
      _location = _impl->next(_location, Current);
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
    if constexpr (Current == Level::Frame)
      return _impl->frame(_location);
    else if constexpr (Current == Level::Packet)
      return _impl->packet(_location);
    else
      return _impl->payload(_location);
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
      if ((int)Current < (int)next_type)
        next = this->_impl->next(next, Current);
    }
    return HierarchicalIterator<Descendants...>(this->_impl, next);
  }

  HierarchicalIterator<Descendants...> _crbegin() const {
    // Figure out what next level is.
    Level below;
    switch (Current) {
    case Level::Frame:
      below = Level::Packet;
      break;
    case Level::Packet:
      below = Level::Payload;
      break;
    default:
      throw std::logic_error("Supposedly unreachable");
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

class Sink;
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
  virtual bool trace(device::ID deviceID, bool enabled = true) = 0;

  virtual bool registerSink(Sink *, Mode) = 0;
  virtual void unregisterSink(Sink *) = 0;

  // Must implicitly call updateFrameHeader to fix back links / lengths.
  virtual bool writeFragment(const frame::Header &) = 0;
  virtual bool writeFragment(const packet::Header &) = 0;
  virtual bool writeFragment(const packet::Payload &) = 0;

  virtual bool updateFrameHeader() = 0;

  // Remove the last frame from the buffer.
  // TODO: replace with integration for iterators / std::erase.
  virtual void dropLast() = 0;

  virtual FrameIterator cbegin() const = 0;
  virtual FrameIterator cend() const = 0;
  virtual FrameIterator crbegin() const = 0;
  virtual FrameIterator crend() const = 0;
};

class Source {
public:
  virtual ~Source() = default;
  virtual void setBuffer(Buffer *tb) = 0;
  virtual void trace(bool enabled) = 0;
};

class Sink {
public:
  virtual ~Sink() = default;
  enum class Direction {
    Forward,  // Apply the action specified by the packet.
    Backward, // Undo the effects of the action specified by the packet.
  };
  // Return true if the packet was processed by this sink, otherwise return
  // false.
  virtual bool analyze(PacketIterator iter, Direction) = 0;
};

} // namespace trace

// In API v1, errors are communicated via an Error field.
// In API v2 (this version), errors are communicated via exceptions.
namespace tick {
using Type = quint32;
struct Result {
  // After this tick, should control be returned to execution enviroment?
  bool pause = false;
  // Number of ticks required before this device can be clock'ed again.
  // If you need a 1 cycle delay, your delay would be 1*source->interval();
  // 2 tick delay is 1*source->interval(), etc.
  Type delay;
};

class Source {
public:
  virtual ~Source() = default;
  // Number of ticks between clock cycles.
  virtual Type interval() const = 0;
};

class Recipient {
public:
  virtual ~Recipient() = default;
  virtual const Source *getSource() = 0;
  virtual void setSource(Source *) = 0;
  virtual Result clock(tick::Type currentTick) = 0;
};
} // namespace tick

// In API v1, errors are communicated via an Error field.
// In API v2 (this version), errors are communicated via exceptions.
namespace memory {

// If select memory operations fail (e.g., lack of MMI, unmapped address in
// bus), specify the behavior of the target.
enum class FailPolicy {
  YieldDefaultValue, // The target picks some arbitrary default value, and
                     // returns it successfully.
  RaiseError         // The target returns an appropriate error message.
};

template <typename Address> class Error : public std::runtime_error {
public:
  enum class Type {
    Unmapped,  // Attempted to read a physical address with no device present.
    OOBAccess, // Attempted out-of-bound access on a storage device.
    NeedsMMI,  // Attempted to read MMI that had no buffered input.
    WriteToRO, // Attempt to write to read-only memory.
  };
  static const std::string format(Address address) {
    auto addrString = QString::number(address, 16);
    return QString("Memory access error at 0x%1").arg(addrString, sizeof(Address) * 2, '0').toStdString();
  }
  Error(Type type, Address address) : std::runtime_error(format(address)), _type(type), _address(address){};
  Address address() const { return _address; }
  Type type() const { return _type; }

private:
  Type _type;
  Address _address;
};

template <typename Address> struct AddressSpan {
  Address minOffset, maxOffset;
};

struct Result {
  // Number of simulation ticks required to complete the memory op.
  // 0 indicates the operation completed immediately.
  tick::Type delay;
  // Did the memory access trigger a breakpoint?
  bool pause;
};

// In API v1, there was a concept of effectful and speculative.
// effectful=false implied that the memory operation was triggered by the UI.
// speculative=true implied some form of memory pre-fetch triggered by the
// simulated hardware. In no case was effectful=false, speculative=true a
// meaningful combination.
//
// In API v2, these have been condensed into a Operation::Type, eliminating the
// impossible case.
struct Operation {
  enum class Type : quint8 {
    // Access triggered by the application / user interface.
    // Should not trigger memory-mapped IO, cache misses, etc.
    Application = 1,
    // Speculative access triggered within the simulation. Probably shouldn't
    // trigger
    // MMIO, but this is hardware dependent.
    Speculative = 2,
    // Access triggered by the simulator while performing some analysis
    // operation.
    // It must never trigger memory-mapped IO nor is it allowed to emit trace
    // events.
    BufferInternal = 3,
    // Non-speculative access triggered within the simulation. Should trigger
    // memory mapped IO,
    // cache updates, etc.
    Standard = 0,
  } type = Type::Standard;
  enum class Kind : bool { instruction = false, data = true } kind;
};

template <typename Address> struct Target {
  virtual ~Target() = default;
  virtual AddressSpan<Address> span() const = 0;
  virtual Result read(Address address, bits::span<quint8> dest, Operation op) const = 0;
  virtual Result write(Address address, bits::span<const quint8> src, Operation op) = 0;
  virtual void clear(quint8 fill) = 0;

  // If dest is larger than maxOffset-minOffset+1, copy bytes from this target
  // to the span.
  virtual void dump(bits::span<quint8> dest) const = 0;
};

template <typename Address> struct Initiator {
  virtual ~Initiator() = default;
  // Sets the memory backing for a particular port (i.e., set the I and D caches
  // separately) If port is nullptr, then all ports will use the target,
  virtual void setTarget(Target<Address> *target, void *port = nullptr) = 0;
};
} // namespace memory

struct Scheduler {
  enum class Mode {
    Increment, // Execute only the next tick, even if no clocked device is
    // ticked.
    Jump, // Execute up to and including the next tick with a clocked device.
  };
  virtual ~Scheduler() = default;
  virtual tick::Recipient *next(tick::Type current, Mode mode) = 0;
  virtual void schedule(tick::Recipient *listener, tick::Type startingOn) = 0;
  virtual void reschedule(device::ID device, tick::Type startingOn) = 0;
};

template <typename Address> struct System {
  virtual ~System() = default;
  // Returns (current tick, result of ticking that clocked device).
  // Implementors are responsible for creating a frame packet in the TB at the
  // start of the function, and updating the frame header packer at the end of
  // the function.
  // TODO: clock all devices who are scheduled for the next tick.
  virtual std::pair<tick::Type, tick::Result> tick(Scheduler::Mode mode) = 0;
  virtual tick::Type currentTick() const = 0;
  virtual device::ID nextID() = 0;
  virtual device::IDGenerator nextIDGenerator() = 0;

  virtual void setBuffer(trace::Buffer *buffer) = 0;
};

} // namespace sim::api2
