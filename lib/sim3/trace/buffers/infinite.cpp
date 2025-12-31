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

#include "infinite.hpp"
#include <variant>
#include "../frame_utils.hpp"

using Fragment = sim::api2::trace::Fragment;

sim::trace2::InfiniteBuffer::InfiniteBuffer() : _in(_data), _out(_data), _backlinks(256) {}

bool sim::trace2::InfiniteBuffer::trace(sim::api2::device::ID deviceID, bool enabled) {
  if (enabled) _sinks.insert(deviceID);
  else _sinks.remove(deviceID);
  return true;
}

bool sim::trace2::InfiniteBuffer::traced(quint16 deviceID) const { return _sinks.contains(deviceID); }

bool sim::trace2::InfiniteBuffer::writeFragment(const sim::api2::trace::Fragment &fragment) {
  if (auto hdr = std::visit(sim::trace2::AsFrameHeader{}, fragment); hdr.index() != 0) {
    // Both will point to same position when header is first element to be serialized.
    if (_lastFrameStart != _out.position()) updateFrameHeader();

    // Zero out length field of header, and set back_offset.
    std::visit(sim::trace2::UpdateFrameLength{0, hdr}, hdr);
    quint16 back_offset = _out.position() - _lastFrameStart;
    std::visit(sim::trace2::UpdateFrameBackOffset{back_offset, hdr}, hdr);

    // Save current offset to enable updateFrameHeader() to overwrite length in the future.
    _lastFrameStart = _out.position();
    _out(as_fragment(hdr)).or_throw();
  } else _out(Fragment(fragment)).or_throw();
  return true;
}

bool sim::trace2::InfiniteBuffer::updateFrameHeader() {
  namespace fh = api2::frame::header;
  using Header = api2::frame::Header;
  auto curOutPos = _out.position();
  auto curInPos = _in.position();

  // Read in previous frame header and update its length flag
  Fragment w;
  _in.reset(_lastFrameStart);
  _in(w).or_throw();
  _in.reset(curInPos);

  if (auto hdr = std::visit(sim::trace2::AsFrameHeader{}, w); hdr.index() != 0) {
    // TODO: Ensure that length fits in 16 bits.
    quint32 length = curOutPos - _lastFrameStart;
    std::visit(sim::trace2::UpdateFrameLength{static_cast<quint16>(length), hdr}, hdr);

    // Overwrite existing frame header to update "length" field.
    _out.reset(_lastFrameStart);
    _out(std::visit(sim::trace2::AsFragment{}, hdr)).or_throw();
    _out.reset(curOutPos);
  } else return false;

  return true;
}

void sim::trace2::InfiniteBuffer::dropLast() {}

void sim::trace2::InfiniteBuffer::clear() {
  sim::api2::trace::Buffer::clear();
  _out.reset();
  _in.reset();
  _lastFrameStart = 0;
  _data.resize(0);
  _backlinks.clear();
}

sim::trace2::InfiniteBuffer::FrameIterator sim::trace2::InfiniteBuffer::cbegin() const {
  return FrameIterator(this, 0);
}

sim::trace2::InfiniteBuffer::FrameIterator sim::trace2::InfiniteBuffer::cend() const {
  return FrameIterator(this, _out.position());
}

sim::trace2::InfiniteBuffer::FrameIterator sim::trace2::InfiniteBuffer::crbegin() const {
  return FrameIterator(this, _lastFrameStart, api2::trace::Direction::Reverse);
}

sim::trace2::InfiniteBuffer::FrameIterator sim::trace2::InfiniteBuffer::crend() const {
  // -1 has arbitrarily been chosen as end sentinel.
  return FrameIterator(this, -1, api2::trace::Direction::Reverse);
}

std::size_t sim::trace2::InfiniteBuffer::size_at(std::size_t loc, api2::trace::Level level) const {
  typename std::remove_const<decltype(_in)>::type in(_data);
  in.reset(loc);

  Fragment w;
  in(w).or_throw();

  return in.position() - loc;
}

sim::api2::trace::Level sim::trace2::InfiniteBuffer::at(std::size_t loc) const {
  typename std::remove_const<decltype(_in)>::type in(_data);
  in.reset(loc);

  Fragment w;
  in(w).or_throw();

  if (std::visit(sim::trace2::IsFrameHeader{}, w)) return api2::trace::Level::Frame;
  else if (std::visit(sim::trace2::IsPacketHeader{}, w)) return api2::trace::Level::Packet;
  else return api2::trace::Level::Payload;
}

sim::api2::frame::Header sim::trace2::InfiniteBuffer::frame(std::size_t loc) const {
  typename std::remove_const<decltype(_in)>::type in(_data);
  in.reset(loc);

  Fragment w;
  in(w).or_throw();
  return std::visit(sim::trace2::AsFrameHeader{}, w);
}

sim::api2::packet::Header sim::trace2::InfiniteBuffer::packet(std::size_t loc) const {
  typename std::remove_const<decltype(_in)>::type in(_data);
  in.reset(loc);

  Fragment w;
  in(w).or_throw();
  return std::visit(sim::trace2::AsPacketHeader{}, w);
}

sim::api2::packet::Payload sim::trace2::InfiniteBuffer::payload(std::size_t loc) const {
  typename std::remove_const<decltype(_in)>::type in(_data);
  in.reset(loc);

  Fragment w;
  in(w).or_throw();
  return std::visit(sim::trace2::AsPacketPayload{}, w);
}

std::size_t sim::trace2::InfiniteBuffer::next(std::size_t loc, api2::trace::Level level, bool allow_jumps) const {
  using api2::trace::Level;
  using namespace api2::trace;
  // Prevents following condition from deref'ing an invalid iterator.
  if (loc == _out.position()) return loc;
  // If we are at a frame and want to go to the next frame, use the length (if not 0).
  else if (allow_jumps && level == Level::Frame && at(loc) == Level::Frame) {
    auto value = frame(loc);
    auto length = std::visit(trace2::GetFrameLength(), value);
    // May be 0 if this is last frame in trace.
    if (length > 0) return loc + length;
  }

  // Track last visited item to enable caching.
  auto prev = loc;

  typename std::remove_const<decltype(_in)>::type in(_data);
  loc += size_at(loc, level);
  in.reset(loc);

  Fragment w;
  while (true) {
    if (loc == _out.position()) return loc;
    loc = in.position();
    auto ret = in(w);
    if (ret.code == std::errc::result_out_of_range) return 0;
    else if (ret.code != std::errc{}) throw std::logic_error("Unhandled");
    // Prevent "going up" to the next level of trace by returning 0.
    _backlinks.insert(loc, prev);
    switch (level) {
    case api2::trace::Level::Frame:
      if (is_frame_header(w)) return loc;
      break;
    case api2::trace::Level::Packet:
      if (is_frame_header(w) || is_packet_header(w)) return loc;
      break;
    case api2::trace::Level::Payload: return loc;
    }
    prev = loc;
    loc = in.position();
  }
}

std::size_t sim::trace2::InfiniteBuffer::last_before(std::size_t start, std::size_t end,
                                                     api2::trace::Level level) const {
  auto loc = start, prev = start;
  while (loc < end) {
    prev = loc;
    loc = next(loc, level, false);
  }
  return prev;
}

std::size_t sim::trace2::InfiniteBuffer::end() const { return _out.position(); }

std::size_t sim::trace2::InfiniteBuffer::next(std::size_t loc, api2::trace::Level level) const {
  return next(loc, level, true);
}

std::size_t sim::trace2::InfiniteBuffer::prev(std::size_t loc, api2::trace::Level level) const {
  using api2::trace::Level;

  // When iterating forward, we can use_out.position as an invalid end sentinel.
  // If we are already at 0, then we are at the beginning of the trace,
  // so we should return our end sentinel, arbitrarily chosen to be -1.
  if (loc == 0) return -1;
  // If we are at the end of the trace, iterate forward from that last-known frame.
  else if (loc == _out.position()) return last_before(_lastFrameStart, loc, level);
  // If we are at a frame and want to go to the previous frame, use the back_offset.
  else if (level == Level::Frame && at(loc) == Level::Frame) {
    sim::api2::frame::Header value = frame(loc);
    auto offset = std::visit(trace2::GetFrameBackOffset(), value);
    return loc - offset;
  }

  while (true) {
    // If the item isn't in the cache, find the next frame and jump backwards
    // to our frame header. Walk from the header to the previous fragment,
    // filling in the cache as we go.
    if (_backlinks.contains(loc)) loc = _backlinks[loc];
    else if (loc == 0) return loc;
    else {
      auto next_frame = next(loc, Level::Frame, true);
      auto prev_frame = prev(next_frame, Level::Frame);
      // calls next(... , ..., false) repeatedly until we reach loc.
      loc = last_before(prev_frame, loc, Level::Payload);
    }
    auto at_level = at(loc);
    // We found our target if the packet current fragment is at or above or target level of abstraction.
    if ((int)at_level <= (int)level) return loc;
  }
}
