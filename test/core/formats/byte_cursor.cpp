/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
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

#include "core/formats/byte_cursor.hpp"
#include <catch.hpp>
#include "core/formats/elf/packed_storage.hpp"
#include "core/formats/elf/packed_types.hpp"

using namespace pepp::bts;

TEST_CASE("Byte cursor", "[scope:dwarf][kind:unit][arch:*]") {
  SECTION("Round-trip serialization tests") {
    BlockStorage storage;
    storage.append(U16<ElfEndian::be>(0x1234));
    CHECK(storage.append_uleb(12857) == 2);
    storage.append_sleb(-129);
    storage.append_cstr("a.pep");
    storage.append(U32<ElfEndian::le>(0xDEADBEEF));
    const auto out = storage.get(0, storage.size());
    // Spot-check the byte order, and the LEB encodings against DWARF 5 Tables 7.7 and 7.8.
    CHECK(out[0] == 0x12);
    CHECK(out[1] == 0x34);
    CHECK(out[2] == 0xB9);
    CHECK(out[3] == 0x64);
    CHECK(out[4] == 0xFF);
    CHECK(out[5] == 0x7E);

    ByteCursor c{out};
    CHECK(c.fixed<U16<ElfEndian::be>>() == 0x1234);
    CHECK(c.uleb() == 12857);
    CHECK(c.sleb() == -129);
    CHECK(c.cstr() == "a.pep");
    CHECK(c.fixed<U32<ElfEndian::le>>() == 0xDEADBEEF);
    CHECK(c.ok());
    CHECK(c.remaining() == 0);
  }

  SECTION("Stop on underflow for fixed-size values") {
    const std::vector<u8> in{0x01, 0x02, 0x03};
    ByteCursor c{in};
    CHECK(c.fixed<U32<ElfEndian::le>>() == 0);
    CHECK_FALSE(c.ok());
    // Later reads that would otherwise succeed still fail.
    CHECK(c.bytes(1).empty());
    CHECK(c.remaining() == 0);
  }
  SECTION("Stop on underflow for LEB128 decoding") {
    const std::vector<u8> in{0x80, 0x80};
    ByteCursor c{in};
    CHECK(c.uleb() == 0);
    CHECK_FALSE(c.ok());
  }
  SECTION("Stop on underflow for null-terminated strings") {
    const std::vector<u8> in{'a', 'b'};
    ByteCursor c{in};
    CHECK(c.cstr().empty());
    CHECK_FALSE(c.ok());
  }
}