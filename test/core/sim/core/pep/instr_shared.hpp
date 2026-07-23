#include "core/sim/cores/cpu/pep_isa.hpp"
#include "core/sim/cores/cpu/pep_isa_instructions.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

// First return value is the system (don't drop it!) and the second is a ptr to the CPU.
inline auto make_cpu() {
  using namespace bits;
  static const PepISA3CPU::Configuration cpu_cfg{Device::Configuration{
                                                     .basename = "cpu",
                                                     .compatible = PepISA3CPU::compatible,
                                                 },
                                                 PepISA3CPU::ISA::Pep10, "/memory"};
  static const System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  static const Dense::Configuration mem_cfg{
      Device::Configuration{
          .basename = "memory",
          .compatible = Dense::compatible,
      },
      0x00,
      AddressSpan(0x0000, 0xffff),
  };
  auto system = std::make_unique<System>(root_cfg);
  auto *mem = system->make_device<Dense>(Dense::Configuration{mem_cfg});
  auto *cpu = system->make_device<PepISA3CPU>(PepISA3CPU::Configuration{cpu_cfg}, system.get());
  system->initialize();
  return std::make_tuple(std::move(system), mem, cpu);
}

static const Operation rw{.type = Operation::Type::Standard, .kind = Operation::Kind::data};

static inline u16 reg(PepISA3CPU *cpu, isa::Pep10::Register reg) { return cpu->read_register(reg); };

static inline u16 csr(PepISA3CPU *cpu, isa::Pep10::CSR csr) { return cpu->read_csr(csr); };
