#pragma once

#include <stdint.h>
#include <set>

#include "components/delta/base.hpp"
#include "components/machine/interface.hpp"
#include "components/machine/processor_model.hpp"
#include "components/storage/block.hpp"
#include "isa/pep10.hpp"
namespace isa::pep10 {

template<bool enable_history>
class LocalProcessor
    : public components::machine::ProcessorModel<uint16_t, uint8_t, uint16_t, uint8_t, bool, enable_history> {
public:
  LocalProcessor() {}
  LocalProcessor(components::machine::MachineProcessorInterface<uint16_t, enable_history, uint8_t,
                                                                isa::pep10::MemoryVector> &owner);

  // Interface that must be implemented by deriving processor models
  result<step::Result> step() override;
  bool can_step_into() const override;
  uint16_t call_depth() const override;

  // Set up / tear down
  void init() override;
  void debug(bool) override;
  void clear(uint16_t reg_fill, bool csr_fill) override;

  // Read / write registers
  uint16_t read_register(uint8_t reg_number) const override;
  void write_register(uint8_t reg_number, uint16_t value) override;
  uint8_t register_count() const override;

  bool read_csr(uint8_t csr_number) const override;
  void write_csr(uint8_t csr_number, bool value) override;
  uint8_t csr_count() const override;

  // Statistics
  uint64_t cycle_count() const override;
  uint64_t instruction_count() const override;

  // Breakpoints
  void add_breakpoint(uint16_t address) override;
  // Returns true if address had a breakpoint, false otherwise. Either way, no breakpoint shall exist at address.
  bool remove_breakpoint(uint16_t address) override;
  void remove_all_breakpoints() override;

  // Todo: How do I coallate deltas for the CPU register bank
  result<std::unique_ptr<components::delta::Base<uint8_t, uint16_t>>> take_register_delta() override;
  result<std::unique_ptr<components::delta::Base<uint8_t, bool>>> take_csr_delta() override;

  // Needed to gather all deltas since last step() call.
  uint64_t last_step_time() const override;

private:
  components::machine::MachineProcessorInterface<uint16_t, enable_history, uint8_t, isa::pep10::MemoryVector> &_owner;
  std::shared_ptr<components::storage::Block<uint8_t, enable_history, uint16_t>> _registers{nullptr};
  std::shared_ptr<components::storage::Block<uint8_t, enable_history, bool>> _csrs{nullptr};
  uint64_t _cycle_count{0}, _last_step_time{0}, _call_depth{0};
  std::set<uint16_t> _breakpoints;

  result<void> unary_dispatch(uint8_t is);
  result<void> nonunary_dispatch(uint8_t is, uint16_t os);

  result<uint8_t> get_byte(uint16_t address) const;
  result<uint8_t> read_byte(uint16_t address) const;
  result<uint16_t> read_word(uint16_t address) const;
  result<void> write_byte(uint16_t address, uint8_t value);
  result<void> write_word(uint16_t address, uint16_t value);

  result<uint16_t> decode_load_operand(const isa::pep10::instruction_definition &is, isa::pep10::addressing_mode mode,
                                       uint16_t addr);
  result<uint16_t> decode_store_operand(const isa::pep10::instruction_definition &is,
                                        isa::pep10::addressing_mode mode, uint16_t addr);
};

/*
 * Format string must be a valid fmtlib / {fmt} expression, using named arguments rather than positional ones.
 * Valid name codes:
 *   A: Value of the  accumulator after executing the current instruction.
 *   X: Value of the index register after executing the current instruction.
 *   SP: Value of the the stack pointer after executing the current instruction.
 *   PC: Value of the program counter after executing the current instruction.
 *   IS: A string representation of the current mnemonic.
 *   OS: The undecoded operand specifier.
 *   ADDR: A string representation of the addressing mode.
 */
template<bool enable_history>
void debug_summary(const LocalProcessor<enable_history> &proc, const std::string &format);

template<bool enable_history>
uint16_t read_register(const LocalProcessor<enable_history> &proc, isa::pep10::Register reg);

template<bool enable_history>
void write_register(LocalProcessor<enable_history> &proc, isa::pep10::Register reg, uint16_t value);

template<bool enable_history> bool read_NZVC(const LocalProcessor<enable_history> &proc, isa::pep10::CSR reg);

template<bool enable_history> void write_NZVC(LocalProcessor<enable_history> &proc, isa::pep10::CSR reg, bool value);

template<bool enable_history> uint8_t read_packed_NZVC(const LocalProcessor<enable_history> &proc);

template<bool enable_history> void write_packed_NZVC(LocalProcessor<enable_history> &proc, uint8_t packed);

}; // namespace isa::pep10

#include "local_processor.tpp"
