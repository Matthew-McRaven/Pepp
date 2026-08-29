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
#include "core/sim/cores/cpu/rv32/rv_regbank.hpp"
#include "core/sim/memory/errors.hpp"
#include "core/sim/system.hpp"

namespace {
// Register N occupies bytes [N*4, N*4+3], least significant first, matching RV
constexpr u8 slot_of(Address at) { return static_cast<u8>(at / RV32RegisterBank::REGISTER_BYTES); }
constexpr unsigned shift_of(Address at) { return 8u * (at % RV32RegisterBank::REGISTER_BYTES); }
} // namespace

RV32RegisterBank::RV32RegisterBank(Configuration config) : Device(), _config(config) {
  _regs.fill(0);
  _pc = 0;
}

void RV32RegisterBank::initialize(System *sys) {
  using SR = RegisterScan::Register;
  auto *scan = sys->register_scan();
  for (u8 i = 0; i < REGISTER_COUNT; ++i) {
    SR r{};
    r.byte_width = 4;
    // x0 must not be set to a value other than 0.
    if (i == 0) r.guest_access = r.host_access = SR::Access::Read;
    r.target = id();
    r.order = bits::hostOrder();
    r.name = riscv::xname(i);
    r.loc = &_regs[i];
    _refs[i] = scan->expose(r);
  }
  // PC is architectural state but not a general-purpose register, and it should be reachable through scan chain.
  SR pc{};
  pc.byte_width = sizeof(_pc);
  pc.target = id();
  pc.order = bits::hostOrder();
  pc.name = "pc";
  pc.loc = &_pc;
  _pc_ref = scan->expose(pc);
}

void RV32RegisterBank::reset() {
  _regs.fill(0);
  _pc = 0;
}

void RV32RegisterBank::set_initiator(Device::ID cpu) {
  _op = Operation(Operation::Type::Standard, Operation::Kind::data, cpu);
}

RegisterScan::RegisterRef RV32RegisterBank::ref(Register reg) const {
  const auto index = static_cast<u8>(reg);
  return index < REGISTER_COUNT ? _refs[index] : RegisterScan::RegisterRef{};
}

RegisterScan::RegisterRef RV32RegisterBank::pc_ref() const { return _pc_ref; }

const Device::Configuration &RV32RegisterBank::config() const { return _config; }
const RV32RegisterBank::Configuration &RV32RegisterBank::casted_config() const { return _config; }
const Device::ID RV32RegisterBank::id() const { return _config.id; }

Device::Type RV32RegisterBank::type() const {
  using namespace bits;
  using T = Device::Type;
  return T::MemoryTarget | T::Traceable;
}

std::unique_ptr<DeviceSerializer> RV32RegisterBank::serializer() const {
  // Should never be called: the CPU builds this with skip_serialize = true.
  throw std::logic_error("RV32RegisterBank is synthetic and has no serialized form");
}

void RV32RegisterBank::set_recorder(const trace::Recorder &recorder) { _trace = recorder; }
bool RV32RegisterBank::can_generate_traces() const { return true; }
void RV32RegisterBank::trace(bool enabled) { _trace.set_traced(enabled); }
bool RV32RegisterBank::traced() const { return _traced; }
void RV32RegisterBank::on_traced_changed(bool enabled) { _traced = enabled; }

// x0..x31 only. PC is architectural but not a general-purpose register, so it is reachable
// through the register scan rather than through an address here.
AddressSpan RV32RegisterBank::span() const { return AddressSpan(0, REGISTER_COUNT * REGISTER_BYTES - 1); }

Target::Result RV32RegisterBank::read(Address address, bits::span<u8> dest, Operation /*op*/) const {
  using E = Error;
  const auto bounds = span();
  const auto max_addr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < bounds.lower() || max_addr > bounds.upper()) throw E(E::Type::OOBAccess, address);
  // Byte at a time, because a caller may start or end part way through a register.
  for (std::size_t i = 0; i < dest.size(); ++i) {
    const Address at = address + static_cast<Address>(i);
    dest[i] = static_cast<u8>(read(static_cast<Register>(slot_of(at))) >> shift_of(at));
  }
  return {};
}

Target::Result RV32RegisterBank::write(Address address, bits::span<const u8> src, Operation op) {
  using E = Error;
  const auto bounds = span();
  const auto max_addr = (address + std::max<Address>(0, src.size() - 1));
  if (address < bounds.lower() || max_addr > bounds.upper()) throw E(E::Type::OOBAccess, address);
  // Read-modify-write a register at a time, so a write spanning several registers emits one
  // trace record each rather than one per byte. A partial write still records the whole
  // register, with the untouched bytes unchanged.
  std::size_t i = 0;
  while (i < src.size()) {
    const auto slot = slot_of(address + static_cast<Address>(i));
    const auto reg = static_cast<Register>(slot);
    u32 value = read(reg);
    while (i < src.size() && slot_of(address + static_cast<Address>(i)) == slot) {
      const Address at = address + static_cast<Address>(i);
      const auto shift = shift_of(at);
      value = (value & ~(u32(0xFF) << shift)) | (u32(src[i]) << shift);
      ++i;
    }
    if (reg != Register::x0) store(reg, value, op);
  }
  return {};
}

void RV32RegisterBank::clear(u8 fill) {
  // TODO: emit a "clear" trace to TB.
  const u32 wide = 0x01010101u * fill;
  _regs.fill(wide);
  // x0 reads as zero whatever the caller asked for; it has no storage to fill.
  _regs[0] = 0;
}

void RV32RegisterBank::dump(bits::span<u8> dest) const {
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  for (std::size_t i = 0; i < dest.size(); ++i) {
    const auto at = static_cast<Address>(i);
    const auto slot = slot_of(at);
    const u32 value = slot < REGISTER_COUNT ? read(static_cast<Register>(slot)) : 0;
    dest[i] = static_cast<u8>(value >> shift_of(at));
  }
}

void RV32RegisterBank::collect_changes(pepp::core::IntervalSet<Address> &changed) const { changed.insert(span()); }

void RV32RegisterBank::clear_changes() {
  // No-op because we always conservatively report that the whole bank changed.
}
