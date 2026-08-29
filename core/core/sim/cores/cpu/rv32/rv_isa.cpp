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
#include "core/sim/cores/cpu/rv32/rv_isa.hpp"
#include <nlohmann/json.hpp>
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

namespace {
Device *create_rv32cpu(const nlohmann::json &self, System *sys, Device *par) {
  RV32CPU::Configuration cfg;
  try {
    parse_standard_fields(self, cfg);
    if (cfg.basename.empty()) throw ParsingError("RV32CPU must have a basename");
    if (!self.contains("target") || self["target"].is_null()) throw ParsingError("RV32CPU must have a target");
    cfg.target = self["target"].get<std::string>();
  } catch (const nlohmann::json::type_error &e) {
    throw ParsingError("Failed to parse RV32CPU: " + std::string(e.what()));
  }
  return sys->make_device<RV32CPU>(par, cfg, sys);
}

void prefill_rv32cpu(nlohmann::json &obj) {
  obj["compatible"] = RV32CPU::compatible;
  obj["basename"];
  obj["target"];
}

void serialize_rv32cpu(nlohmann::json &obj, const System *sys, const Device *self) {
  auto casted = dynamic_cast<const RV32CPU *>(self);
  if (!casted) throw std::logic_error("serialize_rv32cpu called on non-RV32CPU device");
  obj["compatible"] = RV32CPU::compatible;
  obj["basename"] = casted->config().basename;
  obj["target"] = casted->casted_config().target;
}
} // namespace

RV32CPU::RV32CPU(Configuration cfg, System *sys) : _config(cfg) {
  auto make_regs = [](System *sys, Device::ID parent) {
    auto self = dynamic_cast<RV32CPU *>(sys->find_by_id(parent));
    RV32RegisterBank::Configuration cfg;
    cfg.basename = "regs";
    cfg.skip_serialize = true;
    self->_regbank = sys->make_device<RV32RegisterBank>(parent, cfg);
  };
  sys->make_deferred(DeferredDevice{.parent = _config.id, .ctor = make_regs});
}

void RV32CPU::initialize(System *sys) {
  _op_data = Operation(Operation::Type::Standard, Operation::Kind::data, id());
  _op_fetch = Operation(Operation::Type::Standard, Operation::Kind::instruction, id());

  auto dev = sys->find_relative(_config.target, _config.fullname);
  if (!dev) throw std::runtime_error("RV32CPU: could not find target device " + _config.target);
  _target = dev->capability<Target>();
  if (!_target) throw std::runtime_error("RV32CPU: device " + _config.target + " is not a memory target");
  _regbank->set_initiator(id());

  using SR = RegisterScan::Register;
  sys->register_scan()->expose(SR{.byte_width = sizeof(_count.instructions),
                                  .guest_access = SR::Access::Read,
                                  .restore_on_step_back = false,
                                  .kind = SR::Kind::Count,
                                  .visibility = SR::Visibility::Internal,
                                  .target = id(),
                                  .order = bits::hostOrder(),
                                  .name = "icount",
                                  .loc = &_count.instructions});
}

void RV32CPU::reset() {
  _pc = 0;
  _count = {};
}

const Device::Configuration &RV32CPU::config() const { return _config; }
const RV32CPU::Configuration &RV32CPU::casted_config() const { return _config; }
const Device::ID RV32CPU::id() const { return _config.id; }

Device::Type RV32CPU::type() const {
  using namespace bits;
  using T = Device::Type;
  return T::ClockSink | T::Traceable | T::MemoryInitiator;
}

std::unique_ptr<DeviceSerializer> RV32CPU::serializer() const { return make_serializer(); }

std::unique_ptr<DeviceSerializer> RV32CPU::make_serializer() {
  DeviceSerializer s{.parser = create_rv32cpu,
                     .prefill = prefill_rv32cpu,
                     .serialize = serialize_rv32cpu,
                     .compatible = RV32CPU::compatible};
  return std::make_unique<DeviceSerializer>(std::move(s));
}

riscv::rv_instruction2 RV32CPU::fetch() {
  // TODO
  return riscv::rv_instruction2{0u};
}

void RV32CPU::clock_tick(PulseSchedule::PulseIndex idx, u64 tick) {
  // TODO
  _count.instructions += 1;
}

void RV32CPU::set_clock_source(const ClockSource *src) { _clk = src; }
const ClockSource *RV32CPU::clock_source() const { return _clk; }

void RV32CPU::set_recorder(const trace::Recorder &recorder) { _trace = recorder; }
bool RV32CPU::can_generate_traces() const { return true; }
bool RV32CPU::traced() const { return _may_trace; }
void RV32CPU::on_traced_changed(bool enabled) { _may_trace = enabled; }

void RV32CPU::trace(bool enabled) {
  // The CPU holds no addressable state of its own, so delegate to the child bank.
  _trace.set_traced(enabled);
  if (_regbank) _regbank->trace(enabled);
}

u32 RV32CPU::read_register(Register reg) const { return _regbank->read(reg); }
void RV32CPU::write_register(Register reg, u32 value) { _regbank->write(reg, value); }

void RV32CPU::handle(RvOp op, riscv::rv_instruction2 w) {
  // TODO: add body handlers
  switch (op) {
  case RvOp::LUI: break;
  case RvOp::AUIPC: break;

  case RvOp::JAL: break;
  case RvOp::JALR: break;

  case RvOp::BEQ: break;
  case RvOp::BNE: break;
  case RvOp::BLT: break;
  case RvOp::BGE: break;
  case RvOp::BLTU: break;
  case RvOp::BGEU: break;

  case RvOp::LB: break;
  case RvOp::LH: break;
  case RvOp::LW: break;
  case RvOp::LBU: break;
  case RvOp::LHU: break;

  case RvOp::SB: break;
  case RvOp::SH: break;
  case RvOp::SW: break;

  case RvOp::ADDI: break;
  case RvOp::SLTI: break;
  case RvOp::SLTIU: break;
  case RvOp::XORI: break;
  case RvOp::ORI: break;
  case RvOp::ANDI: break;

  case RvOp::SLLI: break;
  case RvOp::SRLI: break;
  case RvOp::SRAI: break;

  case RvOp::ADD: break;
  case RvOp::SUB: break;
  case RvOp::SLL: break;
  case RvOp::SLT: break;
  case RvOp::SLTU: break;
  case RvOp::XOR: break;
  case RvOp::SRL: break;
  case RvOp::SRA: break;
  case RvOp::OR: break;
  case RvOp::AND: break;

  // Ordering is a no-op on a machine with one core and no store buffer.
  case RvOp::FENCE: break;
  case RvOp::FENCE_TSO: break;

  case RvOp::ECALL: break;
  case RvOp::EBREAK: break;

  case RvOp::INVALID: break;
  }
  throw std::logic_error("RV32CPU: instruction not implemented");
}
