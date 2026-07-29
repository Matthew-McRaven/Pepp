#pragma once

#include "core/arch/pep/isa/pep10.hpp"
#include "core/arch/pep/isa/pep_shared_ops.hpp"
#include "core/sim/api/clock.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/api/trace.hpp"
class Dense;

/*
 * The following classes of instructions are still not tested. Those tests require a more complete system model to
 * implement.
 * - All trap entry instructions
 * - All trap exit instructions
 * - The Pep/9 STOP intruction
 * - All Pep/10 illegal / unimplemented instructions
 */
class PepISA3CPU final : public Device, public ClockSink, public Traceable, public Initiator {
public:
  static const inline std::string compatible = "cpu,pep,isa3";
  enum class ISA {
    Pep8 = 1,
    Pep9 = 2,
    Pep10 = 3,
  };

  struct Configuration : public Device::Configuration {
    ISA isa = ISA::Pep10;
    // Name of device to use as the target for memory access. Resolved to Target* during initialize().
    std::string target;
  };
  PepISA3CPU(Configuration cfg, System *sys);
  ~PepISA3CPU() = default;
  PepISA3CPU(PepISA3CPU &&other) noexcept = default;
  PepISA3CPU &operator=(PepISA3CPU &&other) = default;
  PepISA3CPU(const PepISA3CPU &) = delete;
  PepISA3CPU &operator=(const PepISA3CPU &) = delete;

  const Target *target() const;
  Target *target();

  // Device interface
  void initialize(System *) override;
  const Device::Configuration &config() const override;
  const Configuration &casted_config() const;
  const Device::ID id() const override;
  Device::Type type() const override;
  std::unique_ptr<DeviceSerializer> serializer() const override;
  static std::unique_ptr<DeviceSerializer> make_serializer();

  // ClockSink interface
  void clock_tick(PulseSchedule::PulseIndex idx, u64 tick) override;
  void set_clock_source(const ClockSource *src) override;
  const ClockSource *clock_source() const override;

  // Traceable interface
  void set_buffer(Buffer *tb) override;
  const Buffer *buffer() const override;
  bool can_generate_traces() const override;
  void trace(bool enabled) override;
  bool traced() const override;

  void increment_call_depth();
  void decrement_call_depth();

  template <typename RegisterType> u16 read_register(RegisterType reg);
  template <typename RegisterType> void write_register(RegisterType reg, u16 value);
  template <typename CSRType> bool read_csr(CSRType csr);
  template <typename CSRType> void write_csr(CSRType csr, bool value);
  u8 read_packed_csr();
  void write_packed_csr(u8 value);

  Dense *registers() const { return _regbank; }
  Dense *csrs() const { return _csrs; }

private:
  Configuration _config;
  Buffer *_tb = nullptr;
  Dense *_regbank = nullptr, *_csrs = nullptr;
  Target *_target = nullptr;
  isa::OpcodePlane _opcodes;

  void handle(isa::SharedOp opcode);
  const ClockSource *_clk = nullptr;
};

template <typename RegisterType> inline void PepISA3CPU::write_register(RegisterType reg, u16 value) {
  static const Operation op(Operation::Type::Standard, Operation::Kind::data);
  ((Target *)_regbank)->write<u16, bits::host_is_le>(static_cast<u8>(reg) * 2, value, op);
}

template <typename RegisterType> inline u16 PepISA3CPU::read_register(RegisterType reg) {
  static const Operation op(Operation::Type::Standard, Operation::Kind::data);
  return ((Target *)_regbank)->read<u16, bits::host_is_le>(static_cast<u8>(reg) * 2, op).second;
}

template <typename CSRType> inline void PepISA3CPU::write_csr(CSRType csr, bool value) {
  static const Operation op(Operation::Type::Standard, Operation::Kind::data);
  ((Target *)_csrs)->write<u8, bits::host_is_le>(static_cast<u8>(csr), (u8)value, op);
}

template <typename CSRType> inline bool PepISA3CPU::read_csr(CSRType csr) {
  static const Operation op(Operation::Type::Standard, Operation::Kind::data);
  return ((Target *)_csrs)->read<u8, bits::host_is_le>(static_cast<u8>(csr), op).second != 0;
}
