/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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
#include "core/sim/api/memory.hpp"
#include "core/sim/memory/io/fifo.hpp"
#include "core/sim/memory/io/state.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

namespace {
const Operation rw(Operation::Type::Standard, Operation::Kind::data);
constexpr Address MMIO_BASE = 0x80860000;

u8 byte_at(Target *bus, Address at) {
  u8 v = 0;
  bus->read(at, {&v, 1}, rw);
  return v;
}
} // namespace

TEST_CASE("Standard RV32 system", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  auto sys = create_standard_rv32_system();
  REQUIRE(sys != nullptr);
  sys->initialize();

  auto *bus = sys->find_absolute("/bus")->capability<Target>();
  REQUIRE(bus != nullptr);

  SECTION("MMIO registers are 4-byte aligned") {
    auto *char_out = dynamic_cast<FIFORegister *>(sys->find_absolute("/bus/charOut"));
    auto *pwr_off = dynamic_cast<StateRegister *>(sys->find_absolute("/bus/pwrOff"));
    REQUIRE(char_out != nullptr);
    REQUIRE(pwr_off != nullptr);

    const u8 out = 'A';
    bus->write(MMIO_BASE + 4, {&out, 1}, rw);
    REQUIRE(char_out->output().size() == 1);
    CHECK(char_out->output().at(0) == 'A');
    CHECK(!pwr_off->changed());

    const u8 off = 1;
    bus->write(MMIO_BASE + 8, {&off, 1}, rw);
    CHECK(pwr_off->changed());
    // Writing the power register must not have reached the neighbour four bytes below it.
    CHECK(char_out->output().size() == 1);
  }
  SECTION("Program writes to pwrOff stop the clock") {
    auto *clk = sys->find_absolute("/clk")->capability<ClockSource>();
    const u8 off = 1;
    // simulate a loader writing to pwrOff
    bus->write(MMIO_BASE + 8, {&off, 1}, Operation(Operation::Type::Application, Operation::Kind::data));
    CHECK(clk->schedule().enabled);
    bus->write(MMIO_BASE + 8, {&off, 1}, rw);
    CHECK(!clk->schedule().enabled);
    CHECK(std::get<0>(sys->tick()) == Device::ID{});
  }
  SECTION("reads from charIn reach FIFO") {
    auto *char_in = dynamic_cast<FIFORegister *>(sys->find_absolute("/bus/charIn"));
    REQUIRE(char_in != nullptr);
    char_in->input().push('z');
    CHECK(byte_at(bus, MMIO_BASE) == 'z');
  }
  SECTION("RAM covers everything the registers do not") {
    for (const Address at : {Address(0x00000000), Address(MMIO_BASE - 1), Address(MMIO_BASE + 1),
                             Address(MMIO_BASE + 3), Address(MMIO_BASE + 5), Address(MMIO_BASE + 9),
                             Address(0xFFFFFFFF)}) {
      const u8 marker = 0x5A;
      REQUIRE_NOTHROW(bus->write(at, {&marker, 1}, rw));
      CHECK(byte_at(bus, at) == 0x5A);
    }
  }
}
