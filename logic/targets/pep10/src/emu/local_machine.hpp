#pragma once

#include <memory>

#include <outcome.hpp>

#include "components/delta/base.hpp"
#include "components/machine/delta.hpp"
#include "components/machine/interface.hpp"
#include "components/storage/base.hpp"
#include "emu/local_processor.hpp"
#include "isa/pep10.hpp"

namespace isa::pep10 {

template<bool enable_history>
class LocalMachine : public components::machine::MachineProcessorInterface<uint16_t, enable_history, uint8_t,
                                                                           isa::pep10::MemoryVector> {
public:
  // C.67 suppress polymorphic copy/move.
  // Construct processor in place from this.
  LocalMachine(std::shared_ptr<components::storage::Base<uint16_t, enable_history, uint8_t>> memory);
  ~LocalMachine() override = default;

  /*
   * Accessors / modifiers for processor.
   */
  result<step::Result> step();
  /*
   * Implement MachineProcessorInterface.
   */
  // Load default values into SP & PC. Clears CSRS and regs, but leaves memory intact.
  void init() override;
  void begin_simulation() override;
  void end_simulation() override;
  bool halted() const override;
  /* Do not export these functions to JS.
   * We want our API in JS to require accessing components (e.g. machine.getComponent("CPU").readRegister()) to get
   * values. This has several benefits: It gives us more freedom in API design. I don't have to figure out now how to
   * seemlessly swap Pep/9's proc for Pep/10. It allows for more complex systems to be realized. One is allowed to
   * have multiple CPUS/components/memories. This will be helpful when we design the full-system Pep/10 in the future.
   */
  result<uint8_t> get_memory(uint16_t address) const override;
  result<void> set_memory(uint16_t address, uint8_t value) override;
  result<uint8_t> read_memory(uint16_t address) const override;
  result<void> write_memory(uint16_t address, uint8_t value) override;
  uint16_t max_offset() const override;
  // Don't use results here. Failing a register read/write is a fatal error that will crash the progam.
  uint16_t read_register(isa::pep10::Register reg) const;
  void write_register(isa::pep10::Register reg, uint16_t value);
  bool read_csr(isa::pep10::CSR csr) const;
  void write_csr(isa::pep10::CSR csr, bool value);
  uint8_t read_packed_csr() const;
  void write_packed_csr(uint8_t value);
  // End unbindable functions

  result<void> unwind_active_instruction() override;
  uint16_t address_from_vector(isa::pep10::MemoryVector vector) const override;
  void clear_all(uint8_t mem_fill, uint16_t reg_fill, bool csr_fill);
  void clear_memory(uint8_t mem_fill);
  void clear_processor(uint16_t reg_fill, bool csr_fill);

  // Debug / statistics
  void add_breakpoint(uint16_t address);
  // Returns true if address had a breakpoint, false otherwise. Either way, no breakpoint shall exist at address.
  bool remove_breakpoint(uint16_t address);
  void remove_all_breakpoints();

  // Step back serialization / update querries.
  uint64_t cycle_count() const override;
  result<void> save_deltas() override;
  result<void> clear_deltas() override;
  // TODO: Determine how to flatten multiple delta iterators in to a single cohesive one.
  void *deltas_between(uint64_t start, uint64_t end) const override;

  // Needed to get MMIO devices from names.
  void clear_MMIO_addresses() override;
  void register_MMIO_address(const std::string &device_name, uint16_t address) override;
  result<uint16_t> device_address(const std::string &device_name) const override;
  result<components::storage::Input<uint16_t, enable_history, uint8_t> *>
  input_device(const std::string &device_name) override;
  result<components::storage::Output<uint16_t, enable_history, uint8_t> *>
  output_device(const std::string &device_name) override;

private:
  std::shared_ptr<components::storage::Base<uint16_t, enable_history, uint8_t>> _memory;
  std::shared_ptr<isa::pep10::LocalProcessor<enable_history>> _processor;
  std::optional<uint16_t> _pwrOff_address;
  std::map<uint64_t, components::machine::StepDelta<uint16_t, uint8_t, uint8_t, uint16_t, uint8_t, bool>> _deltas;
  std::map<std::string, uint16_t> _mmio_mapping;

  /*
   * Implement MachineProcessorInterface.
   */
  void begin_transaction() override;
  void end_transaction() override;
  void begin_instruction() override;
  void end_instruction() override;
};

// Will "wrap-around" if bytes exceed maximum offset.
template<bool enable_history>
result<void> load_bytes(std::shared_ptr<LocalMachine<enable_history>> machine, const std::vector<uint8_t> &bytes,
                        uint16_t offset);

// Run the machine forever, or until a value is written to the pwrOff port.
template<bool enable_history> result<void> run(std::shared_ptr<LocalMachine<enable_history>> machine);

// Run the machine forever, or until a value is written to the pwrOff port, or a maximum number of steps is exceeded.
template<bool enable_history>
result<step::Result> run(std::shared_ptr<LocalMachine<enable_history>> machine, uint64_t max_timesteps);

} // namespace isa::pep10
#include "local_machine.tpp"
