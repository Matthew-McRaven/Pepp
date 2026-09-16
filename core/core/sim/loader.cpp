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
#include "core/sim/loader.hpp"
#include <array>
#include <stdexcept>
#include "core/sim/debugger/tvm_encoding.hpp"
#include "core/sim/debugger/tvm_interpreter.hpp"
#include "core/sim/system.hpp"

namespace {
// Loads are performed on the machine's behalf rather than the guest's, and should not trigger memory-mapped IO.
const Operation load_op(Operation::Type::Application, Operation::Kind::data);
} // namespace

Loader::Loader(System *sys) : _sys(sys) {
  if (_sys == nullptr) throw std::logic_error("Loader: needs a system to load into");
  auto mgr = _sys->buffer_manager();
  _program = mgr->alloc_buffer();
  if (_program == nullptr) throw std::runtime_error("Loader: out of buffers");
  _start = _program->location();
  auto backend = std::make_unique<tvm::LoaderBackend>(mgr, _sys);
  _backend = backend.get();
  _interpreter = std::make_unique<tvm::Interpreter>(mgr, std::move(backend));
}

Loader::~Loader() = default;

void Loader::append(bits::span<const u8> bytes) {
  if (!_program->can_fit(bytes.size())) throw std::runtime_error("Loader: program does not fit in one buffer");
  _program->append(bytes);
}

bool Loader::set_register(RegisterScan::RegisterRef reg, u64 value) {
  using namespace tvm::EncodedOp;
  auto resolved = _sys->register_scan()->resolve(reg).first;
  if (resolved == nullptr) return false;

  // Immediates are little-endian throughout this ISA, and SETREG requires the payload to be exactly as wide as its
  // destination.
  std::array<u8, sizeof(u64)> payload{};
  for (std::size_t it = 0; it < payload.size(); ++it) payload[it] = static_cast<u8>(value >> (8 * it));
  const auto op = SetReg<false, 4>{.access = load_op.as_u16(), .reg = reg.reg.value, .field = reg.field.value};

  // The encoder takes the payload's width as a template argument, so the register's width picks the form.
  switch (resolved->byte_width) {
  case 1: {
    const auto enc = op.encode(std::array<u8, 1>{payload[0]});
    append({enc.data(), enc.size()});
    return true;
  }
  case 2: {
    const auto enc = op.encode(std::array<u8, 2>{payload[0], payload[1]});
    append({enc.data(), enc.size()});
    return true;
  }
  case 4: {
    const auto enc = op.encode(std::array<u8, 4>{payload[0], payload[1], payload[2], payload[3]});
    append({enc.data(), enc.size()});
    return true;
  }
  case 8: {
    const auto enc = op.encode(payload);
    append({enc.data(), enc.size()});
    return true;
  }
  // The machine moves every register through a u64, so nothing else can be a destination.
  default: return false;
  }
}

bool Loader::copy_word(RegisterScan::RegisterRef reg, Device::ID src, Address address, bool byteswap) {
  using namespace tvm::EncodedOp;
  if (_sys->register_scan()->resolve(reg).first == nullptr) return false;
  const auto enc = MOVMREG<7>{
      .byteswap = byteswap,
      .access = load_op.as_u16(),
      .dst_hi = reg.reg.value,
      .dst_lo = reg.field.value,
      .OFF_hi = static_cast<u16>(address >> 16),
      .OFF_lo = static_cast<u16>(address & 0xFFFF),
      .srcid = src.value,
  }.encode();
  append({enc.data(), enc.size()});
  return true;
}

bool Loader::run() {
  using namespace tvm::EncodedOp;
  // A program the machine can run has to end in a HALT, and appending a second one would leave dead code behind.
  if (!_halted) {
    const auto enc = Halt<0>{}.encode();
    append({enc.data(), enc.size()});
    _halted = true;
  }
  _interpreter->run(_start);
  return _interpreter->stop_cause() == tvm::StopCause::None;
}

tvm::StopCause Loader::stop_cause() const { return _interpreter->stop_cause(); }
