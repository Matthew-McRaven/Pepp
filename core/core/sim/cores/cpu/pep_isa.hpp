#pragma once

#include "core/arch/pep/isa/pep10.hpp"
#include "core/arch/pep/isa/pep_shared_ops.hpp"
#include "core/sim/api/clock.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/api/trace.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/debugger/trace_recorder.hpp"

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
  void reset() override;
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
  void set_recorder(const trace::Recorder &recorder) override;
  bool can_generate_traces() const override;
  bool traced() const override;
  void trace(bool enabled) override;

  void increment_call_depth();
  void decrement_call_depth();

  // Both redirect PC accesses to _pc.
  template <typename RegisterType> u16 read_register(RegisterType reg);
  template <typename RegisterType> void write_register(RegisterType reg, u16 value);
  // Same as above, but does not redirect PC access.
  template <typename RegisterType> u16 read_register_uncached(RegisterType reg);
  template <typename RegisterType> void write_register_uncached(RegisterType reg, u16 value);
  // The flags occupy the low nibble of the CSR bank's single byte, N at bit 3 through C at bit 0.
  static constexpr u8 CSR_MASK = 0x0F;
  // Which bit of that nibble a given flag is.
  template <typename CSRType> static constexpr u8 csr_bit(CSRType csr);
  template <typename CSRType> bool read_csr(CSRType csr);
  template <typename CSRType> void write_csr(CSRType csr, bool value);
  u8 read_packed_csr();
  void write_packed_csr(u8 value);

  Dense *registers() const { return _regbank; }
  Dense *csrs() const { return _csrs; }

  // No longer static const because it embeds this instance's id.
  Operation op_data() const { return Operation(Operation::Type::Standard, Operation::Kind::data, id()); }

  // While an instruction is in flight, this contains the active PC.
  // At the end of an instruction, it will be written back with the updated value.
  u16 read_pc() const { return _pc; }
  void write_pc(u16 value) { _pc = value; }

private:
  struct PerfCounters {
    i16 call_depth = 0;
    u64 instructions = 0;
  } _count = {};
  RegisterScan::RegisterRef _ref_call_depth = {};
  // Only meaningful between the start and end of clock_tick. See read_pc().
  // Coalescing these reads saves 1-2 register writes on most instructions.
  u16 _pc = 0;
  Configuration _config;
  trace::Recorder _trace;
  Dense *_regbank = nullptr, *_csrs = nullptr;
  Target *_target = nullptr;
  isa::OpcodePlane _opcodes;

  void handle(isa::SharedOp opcode);
  const ClockSource *_clk = nullptr;
};

template <typename RegisterType> inline void PepISA3CPU::write_register(RegisterType reg, u16 value) {
  if (reg == RegisterType::PC) _pc = value;
  else write_register_uncached(reg, value);
}

template <typename RegisterType> inline void PepISA3CPU::write_register_uncached(RegisterType reg, u16 value) {
  ((Target *)_regbank)->write<u16, bits::host_is_le>(static_cast<u8>(reg) * 2, value, op_data());
}

template <typename RegisterType> inline u16 PepISA3CPU::read_register(RegisterType reg) {
  if (reg == RegisterType::PC) return _pc;
  else return read_register_uncached(reg);
}

template <typename RegisterType> inline u16 PepISA3CPU::read_register_uncached(RegisterType reg) {
  return ((Target *)_regbank)->read<u16, bits::host_is_le>(static_cast<u8>(reg) * 2, op_data()).second;
}

// All four flags live in one byte, so a single flag is a bit of it rather than a byte of its own. The enum runs
// N, Z, V, C and the packing puts N highest, so the flag's index counts down from the top of the nibble.
template <typename CSRType> constexpr u8 PepISA3CPU::csr_bit(CSRType csr) {
  return static_cast<u8>(1 << (3 - static_cast<u8>(csr)));
}

template <typename CSRType> inline void PepISA3CPU::write_csr(CSRType csr, bool value) {
  // Read-modify-write: the other three flags share the byte and must survive.
  const u8 bit = csr_bit(csr), packed = read_packed_csr();
  write_packed_csr(static_cast<u8>(value ? (packed | bit) : (packed & ~bit)));
}

template <typename CSRType> inline bool PepISA3CPU::read_csr(CSRType csr) {
  return (read_packed_csr() & csr_bit(csr)) != 0;
}
