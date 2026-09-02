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

#include <catch.hpp>
#include <zpp_bits.h>
#include "sim3/trace/buffers/infinite.hpp"
#include "sim3/trace/packet_utils.hpp"

TEST_CASE("Trace buffer iterators", "[scope:sim][kind:unit][arch:*]") {
  sim::trace2::InfiniteBuffer buf;
  std::array<quint8, 16> src, dest;
  for (int it = 0; it < src.size(); it++) {
    src[it] = 0xFE;
    dest[it] = it;
  }
  buf.trace(1, true);
  buf.trace(2, true);
  buf.emitFrameStart();
  buf.emitWrite<quint16>(1, 0, src, dest);
  buf.emitWrite<quint16>(1, 32, src, dest);
  buf.emitWrite<quint16>(1, 64, src, dest);
  buf.emitPureRead<quint16>(1, 0, 16);
  buf.emitFrameStart();
  buf.updateFrameHeader();

  // FORWARD ITERATION
  {
    // Two frames.
    CHECK(std::distance(buf.cbegin(), buf.cend()) == 2);

    // Four packets.
    auto frame = buf.cbegin();
    auto cbeg = frame.cbegin(), cend = frame.cend();
    CHECK(std::distance(frame.cbegin(), frame.cbegin()) == 0);
    CHECK(std::distance(frame.cend(), frame.cend()) == 0);
    CHECK(std::distance(frame.cbegin(), frame.cend()) == 4);

    auto packet = frame.cbegin();
    for (int it = 0; it < 3; it++) {
      // One payload.
      sim::api2::packet::Header header = *packet;
      CHECK(std::holds_alternative<sim::api2::packet::header::Write>(header));
      auto wr = std::get<sim::api2::packet::header::Write>(header);
      CHECK(wr.address.to_address<quint16>() == 32 * it);
      CHECK(std::distance(packet.cbegin(), packet.cbegin()) == 0);
      CHECK(std::distance(packet.cend(), packet.cend()) == 0);
      CHECK(std::distance(packet.cbegin(), packet.cend()) == 1);
      ++packet;
    }

    // Pure read, no payloads
    CHECK(std::distance(packet.cbegin(), packet.cbegin()) == 0);
    CHECK(std::distance(packet.cend(), packet.cend()) == 0);
    CHECK(std::distance(packet.cbegin(), packet.cend()) == 0);
    ++packet;

    CHECK(packet == frame.cend());
  }
  // REVERSE ITERATION
  // Two frames.
  {
    auto b = buf.crbegin();
    auto e = buf.crend();
    CHECK(std::distance(buf.crbegin(), buf.crend()) == 2);
    // Starts with 0 packets.
    auto frame = buf.crbegin();
    // Check forwards and backwards iterators.
    CHECK(std::distance(frame.cbegin(), frame.cbegin()) == 0);
    CHECK(std::distance(frame.cend(), frame.cend()) == 0);
    CHECK(std::distance(frame.cbegin(), frame.cend()) == 0);
    CHECK(std::distance(frame.crbegin(), frame.crbegin()) == 0);
    CHECK(std::distance(frame.crend(), frame.crend()) == 0);
    CHECK(std::distance(frame.crbegin(), frame.crend()) == 0);
    ++frame;

    // Four packets.
    // Check forwards and backwards iterators.
    CHECK(std::distance(frame.cbegin(), frame.cbegin()) == 0);
    CHECK(std::distance(frame.cend(), frame.cend()) == 0);
    CHECK(std::distance(frame.cbegin(), frame.cend()) == 4);
    CHECK(std::distance(frame.crbegin(), frame.crbegin()) == 0);
    CHECK(std::distance(frame.crend(), frame.crend()) == 0);
    CHECK(std::distance(frame.crbegin(), frame.crend()) == 4);
    // Forward iteration within a frame
    {
      auto packet = frame.crbegin();
      for (int it = 0; it < 3; it++) {
        // One payload.
        sim::api2::packet::Header header = *packet;
        CHECK(std::holds_alternative<sim::api2::packet::header::Write>(header));
        auto wr = std::get<sim::api2::packet::header::Write>(header);
        CHECK(wr.address.to_address<quint16>() == 32 * it);
        CHECK(std::distance(packet.cbegin(), packet.cbegin()) == 0);
        CHECK(std::distance(packet.cend(), packet.cend()) == 0);
        CHECK(std::distance(packet.cbegin(), packet.cend()) == 1);
        CHECK(std::distance(packet.crbegin(), packet.crbegin()) == 0);
        CHECK(std::distance(packet.crend(), packet.crend()) == 0);
        CHECK(std::distance(packet.crbegin(), packet.crend()) == 1);
        ++packet;
      }

      // Pure read, no payloads
      CHECK(std::distance(packet.cbegin(), packet.cbegin()) == 0);
      CHECK(std::distance(packet.cend(), packet.cend()) == 0);
      CHECK(std::distance(packet.cbegin(), packet.cend()) == 0);
      CHECK(std::distance(packet.crbegin(), packet.crbegin()) == 0);
      CHECK(std::distance(packet.crend(), packet.crend()) == 0);
      CHECK(std::distance(packet.crbegin(), packet.crend()) == 0);
      ++packet;

      CHECK(packet == frame.crend());
    }
    // Reverse iteration within a frame
    {
      auto packet = frame.cbegin();
      // Pure read, no payloads
      CHECK(std::distance(packet.cbegin(), packet.cbegin()) == 0);
      CHECK(std::distance(packet.cend(), packet.cend()) == 0);
      CHECK(std::distance(packet.cbegin(), packet.cend()) == 0);
      CHECK(std::distance(packet.crbegin(), packet.crbegin()) == 0);
      CHECK(std::distance(packet.crend(), packet.crend()) == 0);
      CHECK(std::distance(packet.crbegin(), packet.crend()) == 0);
      ++packet;

      for (int it = 0; it < 3; it++) {
        // One payload.
        sim::api2::packet::Header header = *packet;
        CHECK(std::holds_alternative<sim::api2::packet::header::Write>(header));
        auto wr = std::get<sim::api2::packet::header::Write>(header);
        CHECK(wr.address.to_address<quint16>() == 32 * (2 - it));
        CHECK(std::distance(packet.cbegin(), packet.cbegin()) == 0);
        CHECK(std::distance(packet.cend(), packet.cend()) == 0);
        CHECK(std::distance(packet.cbegin(), packet.cend()) == 1);
        CHECK(std::distance(packet.crbegin(), packet.crbegin()) == 0);
        CHECK(std::distance(packet.crend(), packet.crend()) == 0);
        CHECK(std::distance(packet.crbegin(), packet.crend()) == 1);
        ++packet;
      }

      CHECK(packet == frame.cend());
    }
  }
}
