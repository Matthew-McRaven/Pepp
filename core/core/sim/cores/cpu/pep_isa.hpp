#pragma once

#include "core/arch/pep/isa/pep_shared.hpp"
#include "core/sim/api/clock.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/api/trace.hpp"
class Dense;

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

private:
  Configuration _config;
  Buffer *_tb = nullptr;
  Dense *_regbank = nullptr, *_csrs = nullptr;
  Target *_target = nullptr;
  isa::detail::OpcodePlane _opcodes;

  void handle(isa::detail::Opcode opcode);
  const ClockSource *_clk = nullptr;
};
