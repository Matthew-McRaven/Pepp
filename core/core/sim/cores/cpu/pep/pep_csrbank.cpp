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
#include "core/sim/cores/cpu/pep/pep_csrbank.hpp"
#include "core/sim/memory/errors.hpp"
#include "core/sim/system.hpp"

PepCSRBank::PepCSRBank(Configuration config) : Device(), _config(config) {}

void PepCSRBank::initialize(System *sys) {
  using SR = RegisterScan::Register;
  using F = SR::Field;
  auto *scan = sys->register_scan();
  // Bit 7 is MSB, 0 is LSB. The flags occupy the low nibble in CSR enum order, so N is bit 3 and C is bit 0.
  auto n = F{.bit_offset = 3, .bit_width = 1, .name = "N"};
  auto z = F{.bit_offset = 2, .bit_width = 1, .name = "Z"};
  auto v = F{.bit_offset = 1, .bit_width = 1, .name = "V"};
  auto c = F{.bit_offset = 0, .bit_width = 1, .name = "C"};
  SR r{};
  r.byte_width = sizeof(_nzvc);
  // Pointer-backed and host order. A single byte has no byte order of its own, but saying host keeps it consistent
  // with the register bank and stops a byteswap being applied to the fields.
  r.order = bits::hostOrder();
  r.target = id();
  r.name = "NZVC";
  r.fields = {n, z, v, c};
  r.loc = &_nzvc;
  _ref = scan->expose(r);
}

void PepCSRBank::reset() { _nzvc = 0; }

void PepCSRBank::set_initiator(Device::ID cpu) {
  _op = Operation(Operation::Type::Standard, Operation::Kind::data, cpu);
}

void PepCSRBank::store(u8 value, Operation op) {
  // Extract only the low nibble, which contains the only defined bits.
  const auto masked = static_cast<u8>(value & MASK);
  // Set whole register rather than separate fields to reduce size of trace.
  if (_traced) _trace.emit_write_register(op, _ref, static_cast<u8>(_nzvc ^ masked));
  _nzvc = masked;
}

void PepCSRBank::write(CSR csr, bool value) {
  // Read-modify-write: the other three flags share the byte and must survive.
  const u8 mask = bit(csr);
  store(static_cast<u8>(value ? (_nzvc | mask) : (_nzvc & ~mask)), _op);
}

void PepCSRBank::write_n(bool value) { write(CSR::N, value); }
void PepCSRBank::write_z(bool value) { write(CSR::Z, value); }
void PepCSRBank::write_v(bool value) { write(CSR::V, value); }
void PepCSRBank::write_c(bool value) { write(CSR::C, value); }

void PepCSRBank::write_packed(u8 value) { store(value, _op); }

const Device::Configuration &PepCSRBank::config() const { return _config; }
const PepCSRBank::Configuration &PepCSRBank::casted_config() const { return _config; }
const Device::ID PepCSRBank::id() const { return _config.id; }

Device::Type PepCSRBank::type() const {
  using namespace bits;
  using T = Device::Type;
  return T::MemoryTarget | T::Traceable;
}

std::unique_ptr<DeviceSerializer> PepCSRBank::serializer() const {
  throw std::logic_error("PepCSRBank is synthetic and has no serialized form");
}

void PepCSRBank::set_recorder(const trace::Recorder &recorder) { _trace = recorder; }
bool PepCSRBank::can_generate_traces() const { return true; }
void PepCSRBank::trace(bool enabled) { _trace.set_traced(enabled); }
bool PepCSRBank::traced() const { return _traced; }
void PepCSRBank::on_traced_changed(bool enabled) { _traced = enabled; }

AddressSpan PepCSRBank::span() const { return AddressSpan(0, 0); }

Target::Result PepCSRBank::read(Address address, bits::span<u8> dest, Operation op) const {
  using E = Error;
  const auto bounds = span();
  const auto max_addr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < bounds.lower() || max_addr > bounds.upper()) throw E(E::Type::OOBAccess, address);
  if (dest.size() > 0) dest[0] = _nzvc;
  return {};
}

Target::Result PepCSRBank::write(Address address, bits::span<const u8> src, Operation op) {
  using E = Error;
  const auto bounds = span();
  const auto max_addr = (address + std::max<Address>(0, src.size() - 1));
  if (address < bounds.lower() || max_addr > bounds.upper()) throw E(E::Type::OOBAccess, address);
  if (src.size() > 0) store(src[0], op);
  return {};
}

void PepCSRBank::clear(u8 fill) {
  // TODO: emit a "clear" trace to TB.
  _nzvc = static_cast<u8>(fill & MASK);
}

void PepCSRBank::dump(bits::span<u8> dest) const {
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  dest[0] = _nzvc;
}

void PepCSRBank::collect_changes(pepp::core::IntervalSet<Address> &changed) const { changed.insert(AddressSpan(0, 0)); }

void PepCSRBank::clear_changes() {
  // No-op because we always conservatively report that the whole bank changed.
}
