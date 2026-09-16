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
#include <array>
#include <catch.hpp>

#include "core/sim/debugger/tvm_interpreter.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

namespace {

// A system with nothing in it but one RAM.
auto make_system() {
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  Dense::Configuration mem_cfg{
      Device::Configuration{.basename = "memory", .compatible = Dense::compatible},
      0x00,
      AddressSpan(0x0000, 0xffff),
  };
  auto system = std::make_unique<System>(root_cfg);
  auto *mem = system->make_device<Dense>(mem_cfg);
  system->initialize();
  return std::make_tuple(std::move(system), mem);
}

const Operation app(Operation::Type::Application, Operation::Kind::data);

//
u16 peek_be(Target *mem, Address at) {
  std::array<u8, 2> bytes{};
  mem->read(at, {bytes.data(), bytes.size()}, app);
  return (u16)(((u16)bytes[0] << 8) | bytes[1]);
}
void poke_be(Target *mem, Address at, u16 v) {
  const std::array<u8, 2> bytes{(u8)(v >> 8), (u8)(v & 0xFF)};
  mem->write(at, {bytes.data(), bytes.size()}, app);
}

// Expose a big-endian register backed by main memory, so that its bytes can be read back directly.
RegisterScan::RegisterRef expose(System &sys, Dense &mem, const char *name, Address at) {
  RegisterScan::Register r{};
  r.order = bits::Order::BigEndian;
  r.byte_width = 2;
  r.guest_access = RegisterScan::Register::ReadWrite;
  r.target = mem.id();
  r.loc = at;
  r.name = name;
  sys.register_scan()->expose(r);
  return *sys.register_scan()->find(name);
}

constexpr Device::ID S{1};    // initiator id
constexpr Address VECTOR = 0x1234; // the word in memory to copy
constexpr Address REG_AT = 0x2000; // where the destination register keeps its bytes

} // namespace

TEST_CASE("tvm::Interpreter: MOVMREG", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  using namespace tvm::EncodedOp;
  auto [sys, mem] = make_system();
  tvm::TraceBuffer tb(sys->buffer_manager());
  Target *target = mem;
  // To avoid creating multiple devices, expose the register at a fixed location in memory.
  auto dst = expose(*sys, *mem, "PC", REG_AT);
  const u16 access = Operation(Operation::Type::Standard, Operation::Kind::data, S).as_u16();

  // Write some known data to memory.
  poke_be(target, VECTOR, 0xDEAD);
  poke_be(target, REG_AT, 0x0000);

  auto program = [&](bool byteswap) {
    const auto enc =
        MOVMREG<7>{
            .byteswap = byteswap,
            .access = access,
            .dst_hi = dst.reg.value,
            .dst_lo = dst.field.value,
            .OFF_hi = (u16)(VECTOR >> 16),
            .OFF_lo = (u16)(VECTOR & 0xFFFF),
            .srcid = mem->id().value,
        }
            .encode();
    tb.begin(S);
    tb.emit_body(S, {enc.data(), enc.size()});
    return tb.commit(S);
  };

  SECTION("Copy without byteswap") {
    auto blaster = sys->make_trace_interpreter();
    blaster->run(program(false));
    CHECK(blaster->stop_cause() == tvm::StopCause::None);
    CHECK(blaster->csrs().F == 0);
    CHECK(peek_be(target, REG_AT) == 0xDEAD);
    CHECK(sys->register_scan()->read<u16>(dst) == 0xDEAD);
  }
  SECTION("Copy with byteswap") {
    auto blaster = sys->make_trace_interpreter();
    blaster->run(program(true));
    CHECK(blaster->stop_cause() == tvm::StopCause::None);
    CHECK(blaster->csrs().F == 0);
    CHECK(peek_be(target, REG_AT) == 0xADDE);
    CHECK(sys->register_scan()->read<u16>(dst) == 0xADDE);
  }
  SECTION("Source memory is unaffected by read") {
    sys->make_trace_interpreter()->run(program(false));
    CHECK(peek_be(target, VECTOR) == 0xDEAD);
  }
  SECTION("Backwards replay fails") {
    auto loc = program(false);
    auto blaster = sys->make_trace_interpreter();
    blaster->backend().set_direction(tvm::Direction::Backward);
    blaster->run(loc);
    CHECK(blaster->stop_cause() == tvm::StopCause::NotInvertible);
    CHECK(peek_be(target, REG_AT) == 0x0000);
  }
}
