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
#include "core/arch/riscv/isa/rvi.hpp"
#include "core/sim/cores/cpu/rv32/rv_i_instructions.hpp"
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
  // TODO: determine if misaligned. If so, throw.
  auto res = _target->read<u32, !bits::host_is_le>(_pc, op_fetch());
  return riscv::rv_instruction2{res.second};
}

void RV32CPU::clock_tick(PulseSchedule::PulseIndex idx, u64 tick) {
  // Create a single record for the entire instruction
  trace::Recorder::Instruction record(_trace, _may_trace);
  // TODO: when function signature changes, use that tick offset instead of this placeholder.
  record.tick(1);
  const u32 init_pc = _regbank->read_pc();
  const auto instr = fetch();
  const auto op = riscv::decode(instr);
  _pc = init_pc + 4; // TODO: increment size determined by instruction length!
  handle(op, instr);
  const auto pc_delta = _pc - init_pc;
  if ((pc_delta & 0b111) == pc_delta) {
    auto r = _regbank->ref_pc();
    if (_may_trace) _trace.emit_incr_register(op_data(), r, static_cast<i16>(pc_delta));
    _regbank->write_pc_untraced(_pc);
  } else _regbank->write_pc(_pc);
  // TODO: handle breakpoints, debug info, etc
  /*if (has_bps && _bp_filter.maybe_contains(_pc)) {
    // Placeholder action to ensure that the bp check is not optimized out.
    _filter_hits += 1;
  }*/
  record.commit();
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
u32 RV32CPU::read_register(riscv::ABIReg reg) const { return _regbank->read((Register)reg); }
void RV32CPU::write_register(riscv::ABIReg reg, u32 value) { _regbank->write((Register)reg, value); }

void RV32CPU::handle(RvOp op, riscv::rv_instruction2 w) {
  // TODO: add body handlers
  switch (op) {
  case RvOp::LUI: return handle_lui(this, w.as<riscv::InstructionU>());
  case RvOp::AUIPC: return handle_auipc(this, w.as<riscv::InstructionU>());

  case RvOp::JAL: return handle_jal(this, w.as<riscv::InstructionJ>());
  case RvOp::JALR: return handle_jalr(this, w.as<riscv::InstructionI>());

  case RvOp::BEQ: return handle_beq(this, w.as<riscv::InstructionB>());
  case RvOp::BNE: return handle_bne(this, w.as<riscv::InstructionB>());
  case RvOp::BLT: return handle_blt(this, w.as<riscv::InstructionB>());
  case RvOp::BGE: return handle_bge(this, w.as<riscv::InstructionB>());
  case RvOp::BLTU: return handle_bltu(this, w.as<riscv::InstructionB>());
  case RvOp::BGEU: return handle_bgeu(this, w.as<riscv::InstructionB>());

  case RvOp::LB: return handle_lb(this, w.as<riscv::InstructionI>());
  case RvOp::LH: return handle_lh(this, w.as<riscv::InstructionI>());
  case RvOp::LW: return handle_lw(this, w.as<riscv::InstructionI>());
  case RvOp::LBU: return handle_lbu(this, w.as<riscv::InstructionI>());
  case RvOp::LHU: return handle_lhu(this, w.as<riscv::InstructionI>());

  case RvOp::SB: return handle_sb(this, w.as<riscv::InstructionS>());
  case RvOp::SH: return handle_sh(this, w.as<riscv::InstructionS>());
  case RvOp::SW: return handle_sw(this, w.as<riscv::InstructionS>());

  case RvOp::ADDI: return handle_addi(this, w.as<riscv::InstructionI>());
  case RvOp::SLTI: return handle_slti(this, w.as<riscv::InstructionI>());
  case RvOp::SLTIU: return handle_sltiu(this, w.as<riscv::InstructionI>());
  case RvOp::XORI: return handle_xori(this, w.as<riscv::InstructionI>());
  case RvOp::ORI: return handle_ori(this, w.as<riscv::InstructionI>());
  case RvOp::ANDI: return handle_andi(this, w.as<riscv::InstructionI>());

  case RvOp::SLLI: return handle_slli(this, w.as<riscv::InstructionI>());
  case RvOp::SRLI: return handle_srli(this, w.as<riscv::InstructionI>());
  case RvOp::SRAI: return handle_srai(this, w.as<riscv::InstructionI>());

  case RvOp::ADD: return handle_add(this, w.as<riscv::InstructionR>());
  case RvOp::SUB: return handle_sub(this, w.as<riscv::InstructionR>());
  case RvOp::SLL: return handle_sll(this, w.as<riscv::InstructionR>());
  case RvOp::SLT: return handle_slt(this, w.as<riscv::InstructionR>());
  case RvOp::SLTU: return handle_sltu(this, w.as<riscv::InstructionR>());
  case RvOp::XOR: return handle_xor(this, w.as<riscv::InstructionR>());
  case RvOp::SRL: return handle_srl(this, w.as<riscv::InstructionR>());
  case RvOp::SRA: return handle_sra(this, w.as<riscv::InstructionR>());
  case RvOp::OR: return handle_or(this, w.as<riscv::InstructionR>());
  case RvOp::AND: return handle_and(this, w.as<riscv::InstructionR>());

  // Ordering is a no-op on a machine with one core and no store buffer.
  case RvOp::FENCE: return handle_fence(this, w.as<riscv::InstructionI>());
  case RvOp::FENCE_TSO: return handle_fence_tso(this, w.as<riscv::InstructionI>());

  case RvOp::ECALL: return handle_ecall(this, w.as<riscv::InstructionI>());
  case RvOp::EBREAK: return handle_ebreak(this, w.as<riscv::InstructionI>());

  case RvOp::INVALID: [[fallthrough]];
  default: throw std::logic_error("RV32CPU: invalid instruction");
  }
  throw std::logic_error("RV32CPU: instruction not implemented");
}
