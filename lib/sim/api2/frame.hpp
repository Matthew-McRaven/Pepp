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

namespace sim::api2::frame {
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
// If you add a type, update Fragment trace/buffer.hpp
using Header = std::variant<std::monostate, header::Trace, header::Extender>;
} // namespace sim::api2::frame
