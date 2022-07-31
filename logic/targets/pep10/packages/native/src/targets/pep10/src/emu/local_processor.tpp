#include "./local_processor.hpp"

#include <algorithm>
#include <iostream>

#include <magic_enum.hpp>

#include "components/delta/grouped.hpp"
#include "components/machine/processor_error.hpp"
#include "components/machine/processor_model.hpp"

static const bool DEBUG_PROC = false;

template<bool enable_history>
isa::pep10::LocalProcessor<enable_history>::LocalProcessor(
    components::machine::MachineProcessorInterface<uint16_t, enable_history, uint8_t, isa::pep10::MemoryVector> &owner)
    : _owner(owner), _registers(std::make_shared<components::storage::Block<uint8_t, enable_history, uint16_t>>(
    magic_enum::enum_count<isa::pep10::Register>())),
      _csrs(std::make_shared<components::storage::Block<uint8_t, enable_history, bool>>(
          magic_enum::enum_count<isa::pep10::CSR>())) {}

template<bool enable_history> result<step::Result> isa::pep10::LocalProcessor<enable_history>::step() {
  static auto ret_helper = [](isa::pep10::LocalProcessor<enable_history> &proc, auto &x) {
    // Unwind active instruction only if history is enabled.
    // If history is enabled, unwind active instruction *must* work.
    if constexpr (enable_history)
      proc._owner.unwind_active_instruction().value();

    if (x.error() == StorageErrc::NoMMInput)
      return result<step::Result>{step::Result::kNeedsMMI};
      // Must clone to convert error from reference to value.
    else
      return result<step::Result>{x.error().clone()};
  };

  using ::isa::pep10::read_register;
  using ::isa::pep10::write_register;
  auto i_locker = _owner.acquire_instruction_lock();

  // Load PC.
  uint16_t pc = read_register(*this, Register::PC);

  // Load instruction spec from memory.
  auto is = read_byte(pc);
  if (is.has_failure())
    return ret_helper(*this, is);
  write_register(*this, Register::IS, is.value());

  // Increment PC
  write_register(*this, Register::PC, ++pc);

  if (isa::pep10::is_opcode_unary(is.value())) {

    auto success = unary_dispatch(is.value());

    static const auto unary_fmt = "{PC} {A} {X} {STAT}, {SP} ||{IS}";
    if constexpr (DEBUG_PROC)
      debug_summary(*this, unary_fmt);

    if (success.has_failure())
      return ret_helper(*this, success);
  } else {
    // Load operand specifier from memory.
    auto os = read_word(pc);
    if (os.has_failure())
      return ret_helper(*this, is);
    write_register(*this, Register::OS, os.value());

    // Increment PC
    write_register(*this, Register::PC, pc + 2);

    auto success = nonunary_dispatch(is.value(), os.value());

    static const auto nonunary_fmt = "{PC} {A} {X} {STAT}, {SP} ||{IS} {OS},{ADDR}";
    if constexpr (DEBUG_PROC)
      debug_summary(*this, nonunary_fmt);

    if (success.has_failure())
      return ret_helper(*this, success);
  }
  // TODO: Don't ignore errors here! I just need to silence compiler warning for now.
  if constexpr (enable_history)
    _owner.save_deltas().value();
  ++_cycle_count;
  // Step returns false if the machine is halted, therefore must negate condition.
  if (_owner.halted())
    return step::Result::kHalted;
  else if (_breakpoints.contains(read_register(*this, Register::PC)))
    return step::Result::kBreakpoint;
  else
    return step::Result::kNominal;
}

template<bool enable_history> bool isa::pep10::LocalProcessor<enable_history>::can_step_into() const {
  using namespace ::isa::pep10;
  using ::isa::pep10::read_register;

  static const auto isa = isa::pep10::isa_definition::get_definition();

  auto pc = read_register(*this, Register::PC);
  // Get rather than read, so as to avoid messing with execution stats.
  auto result_is = get_byte(pc);
  if (!result_is.has_value())
    return false;

  auto is = result_is.value();
  auto mnemon = isa.riproll[is];

  // Only CALL type instructions can be stepped into.
  switch (mnemon.inst->mnemonic) {
  case instruction_mnemonic::USCALL:[[fallthrough]];
  case instruction_mnemonic::CALL:[[fallthrough]];
  case instruction_mnemonic::SCALL:return true;
  default:return false;
  }
}

template<bool enable_history> uint16_t isa::pep10::LocalProcessor<enable_history>::call_depth() const {
  return _call_depth;
}

template<bool enable_history> void isa::pep10::LocalProcessor<enable_history>::init() {
  _cycle_count = 0;
  _last_step_time = 0;
  _call_depth = 0;
  _registers->clear(0);
  _csrs->clear(0);

  // According to spec, must load values from memory vectors into SP, PC.
  auto SP = _owner.address_from_vector(isa::pep10::MemoryVector::kSystem_Stack);
  auto SP_val = read_word(SP).value();
  ::isa::pep10::write_register(*this, isa::pep10::Register::SP, SP_val);
  auto PC = _owner.address_from_vector(isa::pep10::MemoryVector::kDispatcher);
  auto PC_val = read_word(PC).value();
  ::isa::pep10::write_register(*this, isa::pep10::Register::PC, PC_val);
}

template<bool enable_history> void isa::pep10::LocalProcessor<enable_history>::debug(bool) {
  throw std::invalid_argument("Pep/10 ISA model is not yet implemented.");
}

template<bool enable_history>
void isa::pep10::LocalProcessor<enable_history>::clear(uint16_t reg_fill, bool csr_fill) {
  _registers->clear(reg_fill);
  _csrs->clear(csr_fill);
}

// Read / write registers
template<bool enable_history>
uint16_t isa::pep10::LocalProcessor<enable_history>::read_register(uint8_t reg_number) const {
  return _registers->read(reg_number).value();
}

template<bool enable_history>
void isa::pep10::LocalProcessor<enable_history>::write_register(uint8_t reg_number, uint16_t value) {
  _registers->write(reg_number, value).value();
}

template<bool enable_history> uint8_t isa::pep10::LocalProcessor<enable_history>::register_count() const {
  // Max offset is 0 indexed, whereas count is 1 indexed.
  return _registers->max_offset() + 1;
}

template<bool enable_history> bool isa::pep10::LocalProcessor<enable_history>::read_csr(uint8_t csr_number) const {
  return _csrs->read(csr_number).value();
}

template<bool enable_history>
void isa::pep10::LocalProcessor<enable_history>::write_csr(uint8_t csr_number, bool value) {
  _csrs->write(csr_number, value).value();
}

template<bool enable_history> uint8_t isa::pep10::LocalProcessor<enable_history>::csr_count() const {
  // Max offset is 0 indexed, whereas count is 1 indexed.
  return _csrs->max_offset() + 1;
}

// Statistics
template<bool enable_history> uint64_t isa::pep10::LocalProcessor<enable_history>::cycle_count() const {
  return _cycle_count;
}

template<bool enable_history> uint64_t isa::pep10::LocalProcessor<enable_history>::instruction_count() const {
  return _cycle_count;
}

template<bool enable_history> void isa::pep10::LocalProcessor<enable_history>::add_breakpoint(uint16_t address) {
  _breakpoints.insert(address);
}

template<bool enable_history> bool isa::pep10::LocalProcessor<enable_history>::remove_breakpoint(uint16_t address) {
  auto contained = _breakpoints.contains(address);
  _breakpoints.erase(address);
  return contained;
}

template<bool enable_history> void isa::pep10::LocalProcessor<enable_history>::remove_all_breakpoints() {
  _breakpoints.clear();
}

template<bool enable_history>
result<std::unique_ptr<components::delta::Base<uint8_t, uint16_t>>>
isa::pep10::LocalProcessor<enable_history>::take_register_delta() {
  auto result_regs = _registers->take_delta();
  if (result_regs.has_error())
    return result_regs.error().clone();
  return std::move(result_regs.value());
}

template<bool enable_history>
result<std::unique_ptr<components::delta::Base<uint8_t, bool>>>
isa::pep10::LocalProcessor<enable_history>::take_csr_delta() {
  auto result_csr = _csrs->take_delta();
  if (result_csr.has_error())
    return result_csr.error().clone();
  return std::move(result_csr.value());
}

template<bool enable_history> uint64_t isa::pep10::LocalProcessor<enable_history>::last_step_time() const {
  return this->_last_step_time;
}

template<bool enable_history> result<void> isa::pep10::LocalProcessor<enable_history>::unary_dispatch(uint8_t is) {
  using ::isa::pep10::read_NZVC;
  using ::isa::pep10::read_register;
  using ::isa::pep10::write_register;

  auto [instr, addr] = isa::pep10::isa_definition::get_definition().riproll[is];
  uint16_t temp_word, sp, acc, idx, vector_value;
  uint8_t temp_byte;
  result<uint16_t> outcome_word = result<uint16_t>(OUTCOME_V2_NAMESPACE::in_place_type<uint16_t>);
  result<uint8_t> outcome_byte = result<uint8_t>(OUTCOME_V2_NAMESPACE::in_place_type<uint8_t>);
  result<void> outcome_void = result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);

  sp = read_register(*this, Register::SP);
  acc = read_register(*this, Register::A);
  idx = read_register(*this, Register::X);
  switch (instr->mnemonic) {
  case instruction_mnemonic::RET:outcome_word = std::move(read_word(sp));
    if (outcome_word.has_failure())
      return outcome_word.error().clone();
    write_register(*this, Register::PC, outcome_word.value());
    write_register(*this, Register::SP, sp + 2);

    // Prevent negative overflow on _call_depth.
    _call_depth = std::min<decltype(_call_depth)>(0, _call_depth + 1);
    break;

  case instruction_mnemonic::SRET:
    // Only perform single byte read as status bits do not span full word.
    outcome_byte = std::move(read_byte(sp));
    if (outcome_byte.has_failure())
      return outcome_byte.error().clone();
    // Function will automatically mask out bits that don't matter
    write_packed_NZVC(*this, outcome_byte.value());

    outcome_word = std::move(read_word(sp + 1));
    if (outcome_word.has_failure())
      return outcome_word.error().clone();
    write_register(*this, Register::A, outcome_word.value());

    outcome_word = std::move(read_word(sp + 3));
    if (outcome_word.has_failure())
      return outcome_word.error().clone();
    write_register(*this, Register::X, outcome_word.value());

    outcome_word = std::move(read_word(sp + 5));
    if (outcome_word.has_failure())
      return outcome_word.error().clone();
    write_register(*this, Register::PC, outcome_word.value());

    outcome_word = std::move(read_word(sp + 7));
    if (outcome_word.has_failure())
      return outcome_word.error().clone();
    write_register(*this, Register::SP, outcome_word.value());

    // Prevent negative overflow on _call_depth.
    _call_depth = std::min<decltype(_call_depth)>(0, _call_depth + 1);
    break;

  case instruction_mnemonic::MOVSPA:write_register(*this, Register::A, sp);
    break;
  case instruction_mnemonic::MOVASP:write_register(*this, Register::SP, acc);
    break;

  case instruction_mnemonic::MOVFLGA:write_register(*this, Register::A, ::isa::pep10::read_packed_NZVC(*this));
    break;
  case instruction_mnemonic::MOVAFLG:write_packed_NZVC(*this, acc);
    break;

  case instruction_mnemonic::MOVTA:temp_word = read_register(*this, Register::TR);
    write_register(*this, Register::A, temp_word);
    break;

  case instruction_mnemonic::NOP:break;

  case instruction_mnemonic::NOTA:acc = ~acc;
    write_register(*this, Register::A, acc);
    write_NZVC(*this, CSR::N, acc & 0x8000);
    write_NZVC(*this, CSR::Z, acc == 0x0);
    break;
  case instruction_mnemonic::NOTX:idx = ~idx;
    write_register(*this, Register::X, idx);
    write_NZVC(*this, CSR::N, idx & 0x8000);
    write_NZVC(*this, CSR::Z, idx == 0x0);
    break;

  case instruction_mnemonic::NEGA:acc = ~acc + 1;
    write_register(*this, Register::A, acc);
    write_NZVC(*this, CSR::N, acc & 0x8000);
    write_NZVC(*this, CSR::Z, acc == 0x0);
    write_NZVC(*this, CSR::V, acc == 0x8000);
    break;
  case instruction_mnemonic::NEGX:idx = ~idx + 1;
    write_register(*this, Register::X, idx);
    write_NZVC(*this, CSR::N, idx & 0x8000);
    write_NZVC(*this, CSR::Z, idx == 0x0);
    write_NZVC(*this, CSR::V, idx == 0x8000);
    break;

  case instruction_mnemonic::ASLA:
    // Store in temp, because we need acc for status bit computation.
    temp_word = static_cast<uint16_t>(acc << 1);
    write_register(*this, Register::A, temp_word);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    // Signed overflow occurs when the starting & ending values of the high order bit differ (a xor temp == 1).
    // Then shift the result over by 15 places to only keep high order bit (which is the sign).
    write_NZVC(*this, CSR::V, (acc ^ temp_word) >> 15);
    // Carry out if register starts with high order 1.
    write_NZVC(*this, CSR::C, acc & 0x8000);
    break;
  case instruction_mnemonic::ASLX:
    // Store in temp, because we need acc for status bit computation.
    temp_word = static_cast<uint16_t>(idx << 1);
    write_register(*this, Register::X, temp_word);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    // Signed overflow occurs when the starting & ending values of the high order bit differ (a xor temp == 1).
    // Then shift the result over by 15 places to only keep high order bit (which is the sign).
    write_NZVC(*this, CSR::V, (idx ^ temp_word) >> 15);
    // Carry out if register starts with high order 1.
    write_NZVC(*this, CSR::C, idx & 0x8000);
    break;

  case instruction_mnemonic::ASRA:
    // Shift all bits to the right by 1 position. Since using unsigned shift, must explicitly
    // perform sign extension by hand.
    temp_word = static_cast<uint16_t>(acc >> 1 | ((acc & 0x8000) ? 1 << 15 : 0));
    write_register(*this, Register::A, temp_word);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    // Carry out if register starts with low order 1.
    write_NZVC(*this, CSR::C, acc & 0x1);
    break;
  case instruction_mnemonic::ASRX:
    // Shift all bits to the right by 1 position. Since using unsigned shift, must explicitly
    // perform sign extension by hand.
    temp_word = static_cast<uint16_t>(idx >> 1 | ((idx & 0x8000) ? 1 << 15 : 0));
    write_register(*this, Register::X, temp_word);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    // Carry out if register starts with low order 1.
    write_NZVC(*this, CSR::C, idx & 0x1);
    break;

  case instruction_mnemonic::ROLA:
    // Shift the carry in to low order bit.
    temp_word = static_cast<uint16_t>(acc << 1 | (read_NZVC(*this, CSR::C) ? 1 : 0));
    write_register(*this, Register::A, temp_word);
    // Carry out if register starts with high order 1.
    write_NZVC(*this, CSR::C, acc & 0x8000);
    break;
  case instruction_mnemonic::ROLX:
    // Shift the carry in to low order bit.
    temp_word = static_cast<uint16_t>(idx << 1 | (read_NZVC(*this, CSR::C) ? 1 : 0));
    write_register(*this, Register::X, temp_word);
    // Carry out if register starts with high order 1.
    write_NZVC(*this, CSR::C, idx & 0x8000);
    break;

  case instruction_mnemonic::RORA:
    // Shift the carry in to high order bit.
    temp_word = static_cast<uint16_t>(acc >> 1 | (read_NZVC(*this, CSR::C) ? 1 << 15 : 0));
    write_register(*this, Register::A, temp_word);
    // Carry out if register starts with low order 1.
    write_NZVC(*this, CSR::C, acc & 0x1);
    break;
  case instruction_mnemonic::RORX:
    // Shift the carry in to high order bit.
    temp_word = static_cast<uint16_t>(idx >> 1 | (read_NZVC(*this, CSR::C) ? 1 << 15 : 0));
    write_register(*this, Register::X, temp_word);
    // Carry out if register starts with low order 1.
    write_NZVC(*this, CSR::C, idx & 0x1);
    break;

  case instruction_mnemonic::SCALL:
    // TODO: Intentional fallthrough annotation.
  case instruction_mnemonic::USCALL:vector_value = _owner.address_from_vector(MemoryVector::kSystem_Stack);
    outcome_word = std::move(read_word(vector_value));
    if (outcome_word.has_failure())
      return outcome_word.error().clone();

    temp_word = outcome_word.value();

    outcome_void = std::move(write_byte(temp_word - 1, is));
    if (outcome_void.has_failure())
      return outcome_void.error().clone();

    // Writes SP to mem[T-2], mem[T-3].
    outcome_void = std::move(write_word(temp_word - 3, sp));
    if (outcome_void.has_failure())
      return outcome_void.error().clone();

    // Writes to mem[T-4], mem[T-5].
    outcome_void = std::move(write_word(temp_word - 5, read_register(*this, Register::PC)));
    if (outcome_void.has_failure())
      return outcome_void.error().clone();

    // Writes to mem[T-6], mem[T-7].
    outcome_void = std::move(write_word(temp_word - 7, idx));
    if (outcome_void.has_failure())
      return outcome_void.error().clone();

    // Writes to mem[T-8], mem[T-9].
    outcome_void = std::move(write_word(temp_word - 9, acc));
    if (outcome_void.has_failure())
      return outcome_void.error().clone();

    // Writes NZVC to mem[T-10].
    outcome_void = std::move(write_byte(temp_word - 10, ::isa::pep10::read_packed_NZVC(*this)));
    if (outcome_void.has_failure())
      return outcome_void.error().clone();
    write_register(*this, Register::SP, temp_word - 10);

    vector_value = _owner.address_from_vector(MemoryVector::kTrap_Handler);
    outcome_word = std::move(read_word(vector_value));
    if (outcome_word.has_failure())
      return outcome_word.error().clone();
    write_register(*this, Register::PC, outcome_word.value());

    _call_depth += 1;
    break;
  default:return status_code(ProcessorErrc::IllegalUnaryInstruction);
  }
  return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
}

template<bool enable_history>
result<void> isa::pep10::LocalProcessor<enable_history>::nonunary_dispatch(uint8_t is, uint16_t os) {
  using ::isa::pep10::read_NZVC;
  using ::isa::pep10::read_packed_NZVC;
  using ::isa::pep10::read_register;
  using ::isa::pep10::write_NZVC;
  using ::isa::pep10::write_packed_NZVC;
  using ::isa::pep10::write_register;

  auto [instr, addr] = isa::pep10::isa_definition::get_definition().riproll[is];
  uint16_t temp_word, sp, acc, idx, vector_value;
  uint8_t temp_byte;
  result<uint16_t> outcome_word = result<uint16_t>(OUTCOME_V2_NAMESPACE::in_place_type<uint16_t>);
  result<uint8_t> outcome_byte = result<uint8_t>(OUTCOME_V2_NAMESPACE::in_place_type<uint8_t>);
  result<void> outcome_void = result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);

  sp = read_register(*this, Register::SP);
  acc = read_register(*this, Register::A);
  idx = read_register(*this, Register::X);

  result<uint16_t> result_decoded_operand = result<uint16_t>(OUTCOME_V2_NAMESPACE::in_place_type<uint16_t>);
  if (is_store(is))
    result_decoded_operand = decode_store_operand(*instr, addr, os);
  else
    result_decoded_operand = decode_load_operand(*instr, addr, os);
  if (result_decoded_operand.has_error())
    return result_decoded_operand.error().clone();
  uint16_t decoded_operand = result_decoded_operand.value();

  switch (instr->mnemonic) {
  case instruction_mnemonic::BR:write_register(*this, Register::PC, decoded_operand);
    break;
  case instruction_mnemonic::BRLE:
    if (read_NZVC(*this, CSR::N) || read_NZVC(*this, CSR::Z)) {
      write_register(*this, Register::PC, decoded_operand);
    }
    break;
  case instruction_mnemonic::BRLT:
    if (read_NZVC(*this, CSR::N)) {
      write_register(*this, Register::PC, decoded_operand);
    }
    break;
  case instruction_mnemonic::BREQ:
    if (read_NZVC(*this, CSR::Z)) {
      write_register(*this, Register::PC, decoded_operand);
    }
    break;
  case instruction_mnemonic::BRNE:
    if (!read_NZVC(*this, CSR::Z)) {
      write_register(*this, Register::PC, decoded_operand);
    }
    break;
  case instruction_mnemonic::BRGE:
    if (!read_NZVC(*this, CSR::N)) {
      write_register(*this, Register::PC, decoded_operand);
    }
    break;
  case instruction_mnemonic::BRGT:
    if (!read_NZVC(*this, CSR::N) && !read_NZVC(*this, CSR::Z)) {
      write_register(*this, Register::PC, decoded_operand);
    }
    break;
  case instruction_mnemonic::BRV:
    if (read_NZVC(*this, CSR::V)) {
      write_register(*this, Register::PC, decoded_operand);
    }
    break;
  case instruction_mnemonic::BRC:
    if (read_NZVC(*this, CSR::C)) {
      write_register(*this, Register::PC, decoded_operand);
    }
    break;
  case instruction_mnemonic::CALL:sp -= 2;
    outcome_void = std::move(write_word(sp, read_register(*this, Register::PC)));
    if (outcome_void.has_error())
      return outcome_void.error().clone();

    // Load decoded operand as new PC.
    write_register(*this, Register::PC, decoded_operand);
    // Update SP with new value
    write_register(*this, Register::SP, sp);

    _call_depth += 1;
    break;
  case instruction_mnemonic::ADDSP:write_register(*this, Register::SP, sp + decoded_operand);
    break;
  case instruction_mnemonic::SUBSP:write_register(*this, Register::SP, sp - decoded_operand);
    break;

  case instruction_mnemonic::ADDA:
    // The result is the decoded operand specifier plus the accumulator
    temp_word = acc + decoded_operand;
    write_register(*this, Register::A, temp_word);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    // There is a signed overflow iff the high order bits of the register and operand
    // are the same, and one input & the output differ in sign.
    // >> Shifts in 0's (unsigned shorts), so after shift, only high order bit remain.
    write_NZVC(*this, CSR::V, (~(acc ^ decoded_operand) & (acc ^ temp_word)) >> 15);
    // Carry out iff result is unsigned less than register or operand.
    write_NZVC(*this, CSR::C, temp_word < acc || temp_word < decoded_operand);
    break;
  case instruction_mnemonic::ADDX:
    // The result is the decoded operand specifier plus the accumulator
    temp_word = idx + decoded_operand;
    write_register(*this, Register::X, temp_word);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    // There is a signed overflow iff the high order bits of the register and operand
    // are the same, and one input & the output differ in sign.
    // >> Shifts in 0's (unsigned shorts), so after shift, only high order bit remain.
    write_NZVC(*this, CSR::V, (~(idx ^ decoded_operand) & (idx ^ temp_word)) >> 15);
    // Carry out iff result is unsigned less than register or operand.
    write_NZVC(*this, CSR::C, temp_word < idx || temp_word < decoded_operand);
    break;
  case instruction_mnemonic::SUBA:
    // The result is the decoded operand specifier plus the accumulator
    temp_word = acc + ~decoded_operand + 1;
    write_register(*this, Register::A, temp_word);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    // There is a signed overflow iff the high order bits of the register and operand
    // are the same, and one input & the output differ in sign.
    // >> Shifts in 0's (unsigned shorts), so after shift, only high order bit remain.
    write_NZVC(*this, CSR::V, (~(acc ^ decoded_operand) & (acc ^ temp_word)) >> 15);
    // Carry out iff result is unsigned less than register or negated operand.
    write_NZVC(*this, CSR::C, temp_word < acc || temp_word < static_cast<uint16_t>(1 + ~decoded_operand));
    break;
  case instruction_mnemonic::SUBX:
    // The result is the decoded operand specifier plus the accumulator
    temp_word = idx + ~decoded_operand + 1;
    write_register(*this, Register::X, temp_word);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    // There is a signed overflow iff the high order bits of the register and operand
    // are the same, and one input & the output differ in sign.
    // >> Shifts in 0's (unsigned shorts), so after shift, only high order bit remain.
    write_NZVC(*this, CSR::V, (~(idx ^ decoded_operand) & (idx ^ temp_word)) >> 15);
    // Carry out iff result is unsigned less than register or negated operand.
    write_NZVC(*this, CSR::C, temp_word < idx || temp_word < static_cast<uint16_t>(1 + ~decoded_operand));
    break;

  case instruction_mnemonic::ANDA:temp_word = acc & decoded_operand;
    write_register(*this, Register::A, temp_word);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    break;
  case instruction_mnemonic::ANDX:temp_word = idx & decoded_operand;
    write_register(*this, Register::X, temp_word);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    break;
  case instruction_mnemonic::ORA:temp_word = acc | decoded_operand;
    write_register(*this, Register::A, temp_word);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    break;
  case instruction_mnemonic::ORX:temp_word = idx | decoded_operand;
    write_register(*this, Register::X, temp_word);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    break;
  case instruction_mnemonic::XORA:temp_word = acc ^ decoded_operand;
    write_register(*this, Register::A, temp_word);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    break;
  case instruction_mnemonic::XORX:temp_word = idx ^ decoded_operand;
    write_register(*this, Register::X, temp_word);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    break;

  case instruction_mnemonic::CPWA:
    // The result is the decoded operand specifier plus the accumulator
    temp_word = acc + ~decoded_operand + 1;
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    // There is a signed overflow iff the high order bits of the register and operand
    // are the same, and one input & the output differ in sign.
    // >> Shifts in 0's (unsigned shorts), so after shift, only high order bit remain.
    write_NZVC(*this, CSR::V, (~(acc ^ decoded_operand) & (acc ^ temp_word)) >> 15);
    // Carry out iff result is unsigned less than register or operand.
    write_NZVC(*this, CSR::C, temp_word < acc || temp_word < static_cast<uint16_t>(1 + ~decoded_operand));
    // Invert N bit if there was unsigned overflow.
    write_NZVC(*this, CSR::N, read_NZVC(*this, CSR::N) ^ read_NZVC(*this, CSR::V));
    break;
  case instruction_mnemonic::CPWX:
    // The result is the decoded operand specifier plus the accumulator
    temp_word = idx + ~decoded_operand + 1;
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    // There is a signed overflow iff the high order bits of the register and operand
    // are the same, and one input & the output differ in sign.
    // >> Shifts in 0's (unsigned shorts), so after shift, only high order bit remain.
    write_NZVC(*this, CSR::V, (~(idx ^ decoded_operand) & (idx ^ temp_word)) >> 15);
    // Carry out iff result is unsigned less than register or operand.
    write_NZVC(*this, CSR::C, temp_word < idx || temp_word < static_cast<uint16_t>(1 + ~decoded_operand));
    // Invert N bit if there was unsigned overflow.
    write_NZVC(*this, CSR::N, read_NZVC(*this, CSR::N) ^ read_NZVC(*this, CSR::V));
    break;
  case instruction_mnemonic::CPBA:
    // The result is the decoded operand specifier plus the accumulator
    temp_word = acc + ~decoded_operand + 1;
    temp_word &= 0xff;
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x80);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    // RTL specifies that VC get 0.
    write_NZVC(*this, CSR::V, 0);
    write_NZVC(*this, CSR::C, 0);
    break;
  case instruction_mnemonic::CPBX:
    // The result is the decoded operand specifier plus the accumulator
    temp_word = idx + ~decoded_operand + 1;
    temp_word &= 0xff;
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, temp_word & 0x80);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, temp_word == 0);
    // RTL specifies that VC get 0.
    write_NZVC(*this, CSR::V, 0);
    write_NZVC(*this, CSR::C, 0);
    break;
  case instruction_mnemonic::LDWT:
    if (addr != addressing_mode::I)
      return status_code(ProcessorErrc::IllegalAddressingMode);
    write_register(*this, Register::TR, decoded_operand);
    break;
  case instruction_mnemonic::LDWA:write_register(*this, Register::A, decoded_operand);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, decoded_operand & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, decoded_operand == 0);
    break;
  case instruction_mnemonic::LDWX:write_register(*this, Register::X, decoded_operand);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, decoded_operand & 0x8000);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, decoded_operand == 0);
    break;
  case instruction_mnemonic::LDBA:write_register(*this, Register::A, decoded_operand);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, 0);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, decoded_operand == 0);
    break;
  case instruction_mnemonic::LDBX:write_register(*this, Register::X, decoded_operand);
    // Is negative if high order bit is 1.
    write_NZVC(*this, CSR::N, 0);
    // Is zero if all bits are 0's.
    write_NZVC(*this, CSR::Z, decoded_operand == 0);
    break;
  case instruction_mnemonic::STWA:
    outcome_void = std::move(write_word(decoded_operand, read_register(*this, Register::A)));
    if (outcome_void.has_error())
      return outcome_void.error().clone();
    break;
  case instruction_mnemonic::STWX:
    outcome_void = std::move(write_word(decoded_operand, read_register(*this, Register::X)));
    if (outcome_void.has_error())
      return outcome_void.error().clone();
    break;
  case instruction_mnemonic::STBA:
    outcome_void = std::move(write_byte(decoded_operand, read_register(*this, Register::A)));
    if (outcome_void.has_error())
      return outcome_void.error().clone();
    break;
  case instruction_mnemonic::STBX:
    outcome_void = std::move(write_byte(decoded_operand, read_register(*this, Register::X)));
    if (outcome_void.has_error())
      return outcome_void.error().clone();
    break;
  default:return status_code(ProcessorErrc::IllegalNonunaryInstruction);
  }
  return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
}

template<bool enable_history>
result<uint8_t> isa::pep10::LocalProcessor<enable_history>::get_byte(uint16_t address) const {
  auto i = _owner.get_memory(address);
  // Must perform conversion here, because the default move ctor of result produces garbage in some cases.
  if (i.has_failure())
    return i.error().clone();
  return i.value();
}

template<bool enable_history>
result<uint8_t> isa::pep10::LocalProcessor<enable_history>::read_byte(uint16_t address) const {

  auto locker = _owner.acquire_transaction_lock();
  auto i = _owner.read_memory(address);
  // Must perform conversion here, because the default move ctor of result produces garbage in some cases.
  if (i.has_failure())
    return i.error().clone();
  return i.value();
}

template<bool enable_history>
result<uint16_t> isa::pep10::LocalProcessor<enable_history>::read_word(uint16_t address) const {
  auto locker = _owner.acquire_transaction_lock();
  auto msb = _owner.read_memory(address);
  if (msb.has_failure())
    return msb.error().clone();
  auto lsb = _owner.read_memory(address + 1);
  if (lsb.has_failure())
    return lsb.error().clone();

  return msb.value() << 8 | lsb.value();
}

template<bool enable_history>
result<void> isa::pep10::LocalProcessor<enable_history>::write_byte(uint16_t address, uint8_t value) {
  auto locker = _owner.acquire_transaction_lock();
  return _owner.write_memory(address, value);
}

template<bool enable_history>
result<void> isa::pep10::LocalProcessor<enable_history>::write_word(uint16_t address, uint16_t value) {
  auto locker = _owner.acquire_transaction_lock();
  uint8_t msb = ((value & 0xff00) >> 8);
  uint8_t lsb = value & 0xff;
  auto _0 = _owner.write_memory(address, msb);
  if (_0.has_error())
    return _0;
  auto _1 = _owner.write_memory(address + 1, lsb);
  if (_1.has_error())
    return _1;
  else
    return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
}

template<bool enable_history>
result<uint16_t> isa::pep10::LocalProcessor<enable_history>::decode_load_operand(const instruction_definition &instr,
                                                                                 addressing_mode mode, uint16_t addr) {
  using ::isa::pep10::read_register;
  result<uint8_t> outcome_byte = result<uint8_t>(OUTCOME_V2_NAMESPACE::in_place_type<uint8_t>);
  result<uint16_t> outcome_word = result<uint16_t>(OUTCOME_V2_NAMESPACE::in_place_type<uint16_t>);
  if (auto mn = instr.mnemonic; mn == instruction_mnemonic::CPBA || mn == instruction_mnemonic::CPBX ||
      mn == instruction_mnemonic::LDBA || mn == instruction_mnemonic::LDBX) {

    switch (mode) {
    case addressing_mode::I:return addr & 0xff;
    case addressing_mode::D:outcome_byte = std::move(read_byte(addr));
      if (outcome_byte.has_error())
        return outcome_byte.error().clone();
      return outcome_byte.value();
    case addressing_mode::N:outcome_word = std::move(read_word(addr));
      if (outcome_word.has_error())
        return outcome_word.error().clone();
      outcome_byte = std::move(read_byte(outcome_word.value()));
      if (outcome_byte.has_error())
        return outcome_byte.error().clone();
      return outcome_byte.value();
    case addressing_mode::S:outcome_byte = std::move(read_byte(addr + read_register(*this, Register::SP)));
      if (outcome_byte.has_error())
        return outcome_byte.error().clone();
      return outcome_byte.value();
    case addressing_mode::X:outcome_byte = std::move(read_byte(addr + read_register(*this, Register::X)));
      if (outcome_byte.has_error())
        return outcome_byte.error().clone();
      return outcome_byte.value();
    case addressing_mode::SX:addr += read_register(*this, Register::SP) + read_register(*this, Register::X);
      outcome_byte = std::move(read_byte(addr));
      if (outcome_byte.has_error())
        return outcome_byte.error().clone();
      return outcome_byte.value();
    case addressing_mode::SF:outcome_word = std::move(read_word(addr + read_register(*this, Register::SP)));
      if (outcome_word.has_error())
        return outcome_word.error().clone();
      outcome_byte = std::move(read_byte(outcome_word.value()));
      if (outcome_byte.has_error())
        return outcome_byte.error().clone();
      return outcome_byte.value();
    case addressing_mode::SFX:outcome_word = std::move(read_word(addr + read_register(*this, Register::SP)));
      if (outcome_word.has_error())
        return outcome_word.error().clone();
      outcome_byte = std::move(read_byte(outcome_word.value() + +read_register(*this, Register::X)));
      if (outcome_byte.has_error())
        return outcome_byte.error().clone();
      return outcome_byte.value();
    default:
      // Keep the throw. If an illegal enum is passed, we can't recover. Crashing will let us debug.
      throw std::invalid_argument("Not a valid addressing mode");
    }
  } else {
    switch (mode) {
    case addressing_mode::I:return addr;
    case addressing_mode::D:outcome_word = std::move(read_word(addr));
      if (outcome_word.has_error())
        return outcome_word.error().clone();
      return outcome_word.value();
    case addressing_mode::N:outcome_word = std::move(read_word(addr));
      if (outcome_word.has_error())
        return outcome_word.error().clone();
      outcome_word = std::move(read_word(outcome_word.value()));
      if (outcome_word.has_error())
        return outcome_word.error().clone();
      return outcome_word.value();
    case addressing_mode::S:outcome_word = std::move(read_word(addr + read_register(*this, Register::SP)));
      if (outcome_word.has_error())
        return outcome_word.error().clone();
      return outcome_word.value();
    case addressing_mode::X:outcome_word = std::move(read_word(addr + read_register(*this, Register::X)));
      if (outcome_word.has_error())
        return outcome_word.error().clone();
      return outcome_word.value();
    case addressing_mode::SX:addr += read_register(*this, Register::SP) + read_register(*this, Register::X);
      outcome_word = std::move(read_word(addr));
      if (outcome_word.has_error())
        return outcome_word.error().clone();
      return outcome_word.value();
    case addressing_mode::SF:outcome_word = std::move(read_word(addr + read_register(*this, Register::SP)));
      if (outcome_word.has_error())
        return outcome_word.error().clone();
      outcome_word = std::move(read_word(outcome_word.value()));
      if (outcome_word.has_error())
        return outcome_word.error().clone();
      return outcome_word.value();
    case addressing_mode::SFX:outcome_word = std::move(read_word(addr + read_register(*this, Register::SP)));
      if (outcome_word.has_error())
        return outcome_word.error().clone();
      outcome_word = std::move(read_word(outcome_word.value() + +read_register(*this, Register::X)));
      if (outcome_word.has_error())
        return outcome_word.error().clone();
      return outcome_word.value();
    default:
      // Keep the throw. If an illegal enum is passed, we can't recover. Crashing will let us debug.
      throw std::invalid_argument("Not a valid addressing mode");
    }
  }
  // Needed to silence compiler watning
  throw std::invalid_argument("Unreachable");
}

template<bool enable_history>
result<uint16_t> isa::pep10::LocalProcessor<enable_history>::decode_store_operand(const instruction_definition &instr,
                                                                                  addressing_mode mode, uint16_t addr) {
  using ::isa::pep10::read_register;
  result<uint16_t> outcome_word = result<uint16_t>(OUTCOME_V2_NAMESPACE::in_place_type<uint16_t>);
  switch (mode) {
    // Return a non-thrown error when attempting to store immediate.
  case addressing_mode::I:return status_code(ProcessorErrc::IllegalAddressingMode);
  case addressing_mode::D:return addr;
  case addressing_mode::N:return read_word(addr);
  case addressing_mode::S:return addr + read_register(*this, Register::SP);
  case addressing_mode::X:return addr + read_register(*this, Register::X);
  case addressing_mode::SX:addr += read_register(*this, Register::SP) + read_register(*this, Register::X);
    return addr;
  case addressing_mode::SF:return read_word(addr + read_register(*this, Register::SP));
  case addressing_mode::SFX:outcome_word = std::move(read_word(addr + read_register(*this, Register::SP)));
    if (outcome_word.has_error())
      return outcome_word.error().clone();
    return outcome_word.value() + read_register(*this, Register::X);
  default:
    // Keep the throw. If an illegal enum is passed, we can't recover. Crashing will let us debug.
    throw std::invalid_argument("Not a valid addressing mode");
  }
  // Not reachable, but necessary to silence compiler warning
  throw std::invalid_argument("Unreachable");
}

template<bool enable_history>
void isa::pep10::debug_summary(const LocalProcessor <enable_history> &proc, const std::string &format) {
  static const auto &isa = isa::pep10::isa_definition::get_definition();
  std::map<std::string, std::string> args;
  args["A"] = fmt::format("{:04x}", read_register(proc, Register::A));
  args["X"] = fmt::format("{:04x}", read_register(proc, Register::X));
  args["SP"] = fmt::format("{:04x}", read_register(proc, Register::SP));
  args["PC"] = fmt::format("{:04x}", read_register(proc, Register::PC));
  args["STAT"] = fmt::format("{:04b}", read_packed_NZVC(proc));
  auto is = read_register(proc, Register::IS);
  auto instr = isa.riproll[is];
  args["IS"] = fmt::format("{}", isa::pep10::as_string(instr.inst->mnemonic));
  args["OS"] = fmt::format("{:04x}", read_register(proc, Register::OS));
  args["ADDR"] = fmt::format("{}", magic_enum::enum_name(instr.addr));
  std::cout << fmt::vformat(format, fmt::make_format_args(fmt::arg("A", args["A"]),
                                                          fmt::arg("X", args["X"]),
                                                          fmt::arg("SP", args["SP"]),
                                                          fmt::arg("PC", args["PC"]),
                                                          fmt::arg("IS", args["IS"]),
                                                          fmt::arg("OS", args["OS"]),
                                                          fmt::arg("ADDR", args["ADDR"]),
                                                          fmt::arg("STAT", args["STAT"])))
            << std::endl;
}

template<bool enable_history>
uint16_t isa::pep10::read_register(const LocalProcessor <enable_history> &proc, Register reg) {
  return proc.read_register(static_cast<uint8_t>(reg));
}

template<bool enable_history>
void isa::pep10::write_register(LocalProcessor <enable_history> &proc, Register reg, uint16_t value) {
  proc.write_register(static_cast<uint8_t>(reg), value);
}

template<bool enable_history> bool isa::pep10::read_NZVC(const LocalProcessor <enable_history> &proc, CSR reg) {
  return proc.read_csr(static_cast<uint8_t>(reg));
}

template<bool enable_history> void isa::pep10::write_NZVC(LocalProcessor <enable_history> &proc, CSR reg, bool value) {
  proc.write_csr(static_cast<uint8_t>(reg), value);
}

template<bool enable_history> uint8_t isa::pep10::read_packed_NZVC(const LocalProcessor <enable_history> &proc) {
  uint8_t NZVC = 0;
  NZVC |= read_NZVC(proc, CSR::N) << 3;
  NZVC |= read_NZVC(proc, CSR::Z) << 2;
  NZVC |= read_NZVC(proc, CSR::V) << 1;
  NZVC |= read_NZVC(proc, CSR::C) << 0;
  return NZVC;
}

template<bool enable_history>
void isa::pep10::write_packed_NZVC(LocalProcessor <enable_history> &proc, uint8_t packed) {
  write_NZVC(proc, CSR::N, packed & 0b1000);
  write_NZVC(proc, CSR::Z, packed & 0b0100);
  write_NZVC(proc, CSR::V, packed & 0b0010);
  write_NZVC(proc, CSR::C, packed & 0b0001);
}
