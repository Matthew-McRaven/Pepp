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
#include "core/sim/cores/cpu/pep/pep_regbank.hpp"
#include "core/sim/memory/errors.hpp"
#include "core/sim/system.hpp"

namespace {
// Register N occupies bytes [N*2, N*2+1], most significant first. That is the layout the Dense this replaces held,
// so an address-based caller sees exactly what it saw before.
constexpr u8 slot_of(Address at) { return static_cast<u8>(at / 2); }
constexpr bool is_high_byte(Address at) { return (at % 2) == 0; }

} // namespace

PepRegisterBank::PepRegisterBank(Configuration config) : Device(), _config(config) {}

void PepRegisterBank::initialize(System *sys) {
  using SR = RegisterScan::Register;
  auto *scan = sys->register_scan();
  // Pointer-backed and host order: the scan reads the member directly, with no target lookup and no byte swap.
  const auto expose = [&](Register reg, const char *name, auto *slot) {
    SR r{};
    r.byte_width = sizeof(*slot);
    r.order = bits::hostOrder();
    r.target = id();
    r.name = name;
    r.loc = slot;
    _refs[static_cast<u8>(reg)] = scan->expose(r);
  };
  expose(Register::A, "A", &_a);
  expose(Register::X, "X", &_x);
  expose(Register::SP, "SP", &_sp);
  expose(Register::PC, "PC", &_pc);
  expose(Register::IS, "IS", &_is);
  expose(Register::OS, "OS", &_os);
}

void PepRegisterBank::reset() {
  _a = _x = _sp = _pc = _os = 0;
  _is = 0;
}

void PepRegisterBank::set_initiator(Device::ID cpu) {
  _op = Operation(Operation::Type::Standard, Operation::Kind::data, cpu);
}

void PepRegisterBank::write_a(u16 value) { write<Register::A>(value); }
void PepRegisterBank::write_x(u16 value) { write<Register::X>(value); }
void PepRegisterBank::write_sp(u16 value) { write<Register::SP>(value); }
void PepRegisterBank::write_pc(u16 value) { write<Register::PC>(value); }
void PepRegisterBank::write_os(u16 value) { write<Register::OS>(value); }
// Truncating rather than refusing: IS is a byte, and every caller that writes it is handing over an opcode.
void PepRegisterBank::write_is(u8 value) { write<Register::IS>(value); }

RegisterScan::RegisterRef PepRegisterBank::ref(Register reg) const {
  const auto index = static_cast<u8>(reg);
  return index < REGISTER_COUNT ? _refs[index] : RegisterScan::RegisterRef{};
}

const Device::Configuration &PepRegisterBank::config() const { return _config; }
const PepRegisterBank::Configuration &PepRegisterBank::casted_config() const { return _config; }
const Device::ID PepRegisterBank::id() const { return _config.id; }

Device::Type PepRegisterBank::type() const {
  using namespace bits;
  using T = Device::Type;
  return T::MemoryTarget | T::Traceable;
}

std::unique_ptr<DeviceSerializer> PepRegisterBank::serializer() const {
  // Should never be called: the CPU builds this with skip_serialize = true.
  throw std::logic_error("PepRegisterBank is synthetic and has no serialized form");
}

void PepRegisterBank::set_recorder(const trace::Recorder &recorder) { _trace = recorder; }
bool PepRegisterBank::can_generate_traces() const { return true; }
void PepRegisterBank::trace(bool enabled) { _trace.set_traced(enabled); }
bool PepRegisterBank::traced() const { return _traced; }
void PepRegisterBank::on_traced_changed(bool enabled) { _traced = enabled; }

AddressSpan PepRegisterBank::span() const { return AddressSpan(0, REGISTER_COUNT * 2 - 1); }

Target::Result PepRegisterBank::read(Address address, bits::span<u8> dest, Operation op) const {
  using E = Error;
  const auto bounds = span();
  const auto max_addr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < bounds.lower() || max_addr > bounds.upper()) throw E(E::Type::OOBAccess, address);
  // Byte at a time, because a caller may start or end mid-register and the halves come from one host-order member.
  for (std::size_t i = 0; i < dest.size(); ++i) {
    const Address at = address + static_cast<Address>(i);
    const u16 value = read(static_cast<Register>(slot_of(at)));
    dest[i] = is_high_byte(at) ? static_cast<u8>(value >> 8) : static_cast<u8>(value & 0xFF);
  }
  return {};
}

Target::Result PepRegisterBank::write(Address address, bits::span<const u8> src, Operation op) {
  using E = Error;
  const auto bounds = span();
  const auto max_addr = (address + std::max<Address>(0, src.size() - 1));
  if (address < bounds.lower() || max_addr > bounds.upper()) throw E(E::Type::OOBAccess, address);
  // Read-modify-write per register with one trace emitted per register. A write covering half a reigster needs to emit
  // a full SETREGX, with the unused half being 0s. handle_sret exists to reduce the number of traces emitted for bulk
  // register writeback.
  std::size_t i = 0;
  while (i < src.size()) {
    const auto slot = slot_of(address + static_cast<Address>(i));
    const auto reg = static_cast<Register>(slot);
    u16 value = read(reg);
    // Consume every byte of this register present in src before recording anything.
    while (i < src.size() && slot_of(address + static_cast<Address>(i)) == slot) {
      const Address at = address + static_cast<Address>(i);
      value = is_high_byte(at) ? static_cast<u16>((value & 0x00FF) | (u16(src[i]) << 8))
                               : static_cast<u16>((value & 0xFF00) | src[i]);
      ++i;
    }
    write_slot(reg, value, op);
  }
  return {};
}

void PepRegisterBank::write_slot(Register reg, u16 value, Operation op) {
  switch (reg) {
  case Register::A: store(reg, _a, value, op); break;
  case Register::X: store(reg, _x, value, op); break;
  case Register::SP: store(reg, _sp, value, op); break;
  case Register::PC: store(reg, _pc, value, op); break;
  case Register::OS: store(reg, _os, value, op); break;
  case Register::IS: store(reg, _is, static_cast<u8>(value & 0xFF), op); break;
  default: break;
  }
}

void PepRegisterBank::clear(u8 fill) {
  // TODO: emit a "clear" trace to TB.
  const auto wide = static_cast<u16>((u16(fill) << 8) | fill);
  _a = _x = _sp = _pc = _os = wide;
  _is = fill;
}

void PepRegisterBank::dump(bits::span<u8> dest) const {
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  for (std::size_t i = 0; i < dest.size(); ++i) {
    const auto slot = slot_of(static_cast<Address>(i));
    const u16 value = slot < REGISTER_COUNT ? read(static_cast<Register>(slot)) : 0;
    dest[i] = is_high_byte(static_cast<Address>(i)) ? static_cast<u8>(value >> 8) : static_cast<u8>(value & 0xFF);
  }
}

void PepRegisterBank::collect_changes(pepp::core::IntervalSet<Address> &changed) const { changed.insert(span()); }

void PepRegisterBank::clear_changes() {
  // No-op because we always conservatively report that the whole bank changed.
}
