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

#include "sim3/subsystems/disk/ide.hpp"
namespace {
namespace api2 = sim::api2;
auto dense = api2::device::Descriptor{.id = 1, .compatible = nullptr, .baseName = "RAM", .fullName = "/RAM"};
auto desc = api2::device::Descriptor{.id = 2, .compatible = nullptr, .baseName = "IDE", .fullName = "/IDE"};
auto op = api2::memory::Operation{
    .type = api2::memory::Operation::Type::Standard,
    .kind = api2::memory::Operation::Kind::data,
};

void compare(const quint8 *lhs, const quint8 *rhs, quint8 length) {
  if (lhs == nullptr || rhs == nullptr) return;
  for (int it = 0; it < length; it++) CHECK(lhs[it] == rhs[it]);
};
} // namespace

TEST_CASE("IDE Controller sizing check", "[scope:sim][kind:int][arch:*]") {
  int id = desc.id;
  auto nextId = [&id]() { return ++id; };
  // Initialize an IDE controller to a fixed value
  sim::memory::IDEController dev(desc, 0x10, nextId);
  // Compensate for off-by-one due to inclusivity
  CHECK(dev.span().upper() - dev.span().lower() + 1 == 8);
}

TEST_CASE("IDE Controller storage out-of-bounds access", "[scope:sim][kind:int][arch:*][!throws]") {
  int id = desc.id;
  auto nextId = [&id]() { return ++id; };
  // Initialize an IDE controller to a fixed value
  sim::memory::IDEController dev(desc, 0x10, nextId);

  // Create an 8-byte temporary buffer.
  quint64 reg = 0;
  quint8 *tmp = (quint8 *)&reg;
  // In-bound read does not throw, and retrives default value.
  REQUIRE_NOTHROW(dev.read(0x10, {tmp, 1}, op));
  REQUIRE_NOTHROW(dev.read(0x17, {tmp, 1}, op));

  // Initialize tmp to be different than dev default value.
  // Neither OOB read should update temp.
  *tmp = 0xCA;
  REQUIRE_THROWS_AS(dev.read(0x9, {tmp, 1}, op), api2::memory::Error);
  CHECK(*tmp == 0xCA);
  REQUIRE_THROWS_AS(dev.read(0x10 + 8, {tmp, 1}, op), api2::memory::Error);
  CHECK(*tmp == 0xCA);

  // Neither write will stick, so tmp is meaningless
  *tmp = 0xfe;
  REQUIRE_THROWS_AS(dev.write(0x9, {tmp, 1}, op), api2::memory::Error);
  REQUIRE_THROWS_AS(dev.write(0x10 + 8, {tmp, 1}, op), api2::memory::Error);
}

using C = sim::memory::IDEController::Commands;
TEST_CASE("IDE Controller ERASE command", "[scope:sim][kind:int][arch:*]") {
  int id = desc.id;
  auto nextId = [&id]() { return ++id; };
  // Initialize an IDE controller to a fixed value
  sim::memory::IDEController dev(desc, 0x0, nextId);

  // Create 8-byte temporary buffers.
  quint64 reg[3] = {quint64(-1), quint64(-1), quint64(-1)};
  quint8 *tmp = (quint8 *)&reg;
  quint8 *tmp2 = (quint8 *)&reg[1];
  SECTION("Single sector") {
    // Put data across two sectors of the disk.
    for (int i = 0; i < 512; i += 8) {
      REQUIRE_NOTHROW(dev.disk()->write(i, {tmp, 8}, op));
      // Clear buffer to that we know read-back actually works.
      reg[1] = 0;
      REQUIRE_NOTHROW(dev.disk()->read(i, {tmp2, 8}, op));
      CHECK(reg[1] == -1);
    }
    // Clear only the first sector
    dev.setRegs(
        {
            .ideCMD = (int)C::ERASE,
            .offLBA = 0,
            .LBA = 0,
            .addrDMA = 0,
            .lenDMA = 256,
        },
        true);
    // First sector should be 0, second sector should be -1
    for (int i = 0; i < 512; i += 8) {
      // Clear buffer to that we know read-back actually works.
      reg[0] = 7;
      REQUIRE_NOTHROW(dev.disk()->read(i, {tmp, 8}, op));
      if (i < 256) CHECK(reg[0] == 0);
      else CHECK(reg[0] == -1);
    }
  }
  SECTION("Within a sector") {
    // Put some data on the disk.
    REQUIRE_NOTHROW(dev.disk()->write(0, {tmp, 24}, op));
    // Clear buffer to that we know read-back actually works.
    reg[0] = reg[1] = reg[2] = 0;
    REQUIRE_NOTHROW(dev.disk()->read(0, {tmp, 24}, op));
    CHECK((reg[0] == -1 && reg[1] == -1 && reg[2] == -1));
    dev.setRegs(
        {
            .ideCMD = (int)C::ERASE,
            .offLBA = 8,
            .LBA = 0,
            .addrDMA = 0,
            .lenDMA = 8,
        },
        true);
    REQUIRE_NOTHROW(dev.disk()->read(0, {tmp, 24}, op));
    CHECK((reg[0] == -1 && reg[1] == 0 && reg[2] == -1));
  }
  SECTION("Cross sectors") {
    // Put data across two sectors of the disk.
    for (int i = 0; i < 512; i += 8) {
      REQUIRE_NOTHROW(dev.disk()->write(i, {tmp, 8}, op));
      // Clear buffer to that we know read-back actually works.
      reg[1] = 0;
      REQUIRE_NOTHROW(dev.disk()->read(i, {tmp2, 8}, op));
      CHECK(reg[1] == -1);
    }
    dev.setRegs(
        {
            .ideCMD = (int)C::ERASE,
            .offLBA = 128,
            .LBA = 0,
            .addrDMA = 0,
            .lenDMA = 256,
        },
        true);
    // 2nd half of first sector  and 1st half of second sector should be 0. Otherwise -1.
    for (int i = 0; i < 512; i += 8) {
      // Clear buffer to that we know read-back actually works.
      reg[0] = 7;
      REQUIRE_NOTHROW(dev.disk()->read(i, {tmp, 8}, op));
      if (i >= 128 && i < 384) CHECK(reg[0] == 0);
      else CHECK(reg[0] == -1);
    }
  }
}

TEST_CASE("IDE Controller READ command", "[scope:sim][kind:int][arch:*]") {
  int id = desc.id;
  auto nextId = [&id]() { return ++id; };
  // Initialize an IDE controller to a fixed value
  sim::memory::IDEController dev(desc, 0x0, nextId);
  // Use non-0 offset to make sure we don't explode the IDE controller or something.
  sim::memory::Dense<quint16> ram(dense, {256, 512 + 256}, 0);
  dev.setTarget(&ram, nullptr);

  // Create 8-byte temporary buffers.
  quint64 reg0 = -1, reg1 = -1;
  quint8 *tmp = (quint8 *)&reg0;
  quint8 *tmp2 = (quint8 *)&reg1;

  SECTION("Single sector") {
    ram.clear(0);
    // Put data across two sectors of the disk.
    reg0 = -1;
    for (int i = 0; i < 512; i += 8) {
      REQUIRE_NOTHROW(dev.disk()->write(i, {tmp, 8}, op));
    }
    dev.setRegs(
        {
            .ideCMD = (int)C::READ_DMA,
            .offLBA = 0,
            .LBA = 1,
            .addrDMA = 256,
            .lenDMA = 256,
        },
        true);
    // First sector should be 0, second sector should be -1
    for (int i = 0; i < 512; i += 8) {
      // Clear buffer to that we know read-back actually works.
      reg0 = 7;
      REQUIRE_NOTHROW(ram.read(256 + i, {tmp, 8}, op));
      if (i < 256) CHECK(reg0 == -1);
      else CHECK(reg0 == 0);
    }
  }
  SECTION("Cross sectors") {
    ram.clear(0);
    reg0 = -1, reg1 = 0;
    // Put data across two sectors of the disk.
    for (int i = 0; i < 512; i += 8) {
      REQUIRE_NOTHROW(dev.disk()->write(i, {tmp, 8}, op));
      REQUIRE_NOTHROW(dev.disk()->write(512 + i, {tmp2, 8}, op));
    }
    dev.setRegs(
        {
            .ideCMD = (int)C::READ_DMA,
            // Only 1/2 sector of non-0 values will be read in.
            .offLBA = 128,
            .LBA = 1,
            .addrDMA = 256,
            .lenDMA = 256,
        },
        true);
    // First sector should be 0, second sector should be -1
    for (int i = 0; i < 512; i += 8) {
      // Clear buffer to that we know read-back actually works.
      reg0 = 7;
      REQUIRE_NOTHROW(ram.read(256 + i, {tmp, 8}, op));
      if (i < 128) CHECK(reg0 == -1);
      else CHECK(reg0 == 0);
    }
  }
}

TEST_CASE("IDE Controller WRITE command", "[scope:sim][kind:int][arch:*]") {
  int id = desc.id;
  auto nextId = [&id]() { return ++id; };
  // Initialize an IDE controller to a fixed value
  sim::memory::IDEController dev(desc, 0x0, nextId);
  sim::memory::Dense<quint16> ram(dense, {0, 512}, 0);
  dev.setTarget(&ram, nullptr);

  // Create an 8-byte temporary buffer.
  quint64 reg0 = -1, reg1 = -1;
  quint8 *tmp = (quint8 *)&reg0;
  quint8 *tmp2 = (quint8 *)&reg1;
  SECTION("Cross sectors with offset") {
    ram.clear(0);
    // Put data across two sectors of the disk.
    reg0 = 0;
    reg1 = -1;
    for (int i = 0; i < 512; i += 8) {
      REQUIRE_NOTHROW(dev.disk()->write(i, {tmp, 8}, op));
      REQUIRE_NOTHROW(ram.write(i, {tmp2, 8}, op));
    }
    dev.setRegs(
        {
            .ideCMD = (int)C::WRITE_DMA,
            .offLBA = 128,
            .LBA = 1, // 128+256=384, starting address for non-0 bytes.
            .addrDMA = 0,
            .lenDMA = 256, // 384+256=640, ending address for non-0 bytes.
        },
        true);
    // First sector should be 0, second sector should be -1
    for (int i = 0; i < 768; i += 8) {
      // Clear buffer to that we know read-back actually works.
      reg0 = 7;
      REQUIRE_NOTHROW(dev.disk()->read(i, {tmp, 8}, op));
      if (i >= 384 && i < 640) CHECK(reg0 == -1);
      else CHECK(reg0 == 0);
    }
  }
}
