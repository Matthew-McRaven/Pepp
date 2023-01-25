#include <outcome.hpp>

#include <boost/range/adaptor/indexed.hpp>

#include "components/machine/machine_error.hpp"
#include "local_machine.hpp"

template <bool enable_history>
isa::pep10::LocalMachine<enable_history>::LocalMachine(
    std::shared_ptr<components::storage::Base<uint16_t, enable_history, uint8_t>> memory)
    : components::machine::MachineProcessorInterface<uint16_t, enable_history, uint8_t, isa::pep10::MemoryVector>(),
      _memory(memory), _processor(nullptr), _mmio_mapping() {
    using MPI =
        components::machine::MachineProcessorInterface<uint16_t, enable_history, uint8_t, isa::pep10::MemoryVector>;
    _processor = std::make_shared<isa::pep10::LocalProcessor<enable_history>>(static_cast<MPI &>(*this));
}

template <bool enable_history> result<step::Result> isa::pep10::LocalMachine<enable_history>::step() {

    auto ret = _processor->step();
    return ret;
}

// Load default values into SP & PC.
template <bool enable_history> void isa::pep10::LocalMachine<enable_history>::init()
{
    _processor->init();
}

template <bool enable_history> void isa::pep10::LocalMachine<enable_history>::begin_simulation() {
    if (auto pwr_val = device_address("pwrOff"); pwr_val.has_value())
        _pwrOff_address = pwr_val.value();
    else
        _pwrOff_address = std::nullopt;

    _processor->init();
}

template <bool enable_history> void isa::pep10::LocalMachine<enable_history>::end_simulation() {}

template <bool enable_history> bool isa::pep10::LocalMachine<enable_history>::halted() const {
    // Machine is considered halted if a non-zero value has been written to the power off port.
    if (!_pwrOff_address)
        return false;
    else if (auto mem_val = get_memory(*_pwrOff_address); mem_val.has_value())
        return mem_val.value() != 0x00;
    else
        return false;
}

template <bool enable_history>
result<uint8_t> isa::pep10::LocalMachine<enable_history>::get_memory(uint16_t address) const {
    return _memory->get(address);
}

template <bool enable_history>
result<void> isa::pep10::LocalMachine<enable_history>::set_memory(uint16_t address, uint8_t value) {
    return _memory->set(address, value);
}

template <bool enable_history>
result<uint8_t> isa::pep10::LocalMachine<enable_history>::read_memory(uint16_t address) const {
    return _memory->read(address);
}

template <bool enable_history>
result<void> isa::pep10::LocalMachine<enable_history>::write_memory(uint16_t address, uint8_t value) {
    return _memory->write(address, value);
}

template <bool enable_history> uint16_t isa::pep10::LocalMachine<enable_history>::max_offset() const {
    return _memory->max_offset();
}

template <bool enable_history>
uint16_t isa::pep10::LocalMachine<enable_history>::read_register(isa::pep10::Register reg) const {
    return isa::pep10::read_register(*_processor.get(), reg);
}

template <bool enable_history>
void isa::pep10::LocalMachine<enable_history>::write_register(isa::pep10::Register reg, uint16_t value) {
    return isa::pep10::write_register(*_processor.get(), reg, value);
}

template <bool enable_history> bool isa::pep10::LocalMachine<enable_history>::read_csr(isa::pep10::CSR csr) const {
    return isa::pep10::read_NZVC(*_processor.get(), csr);
}

template <bool enable_history>
void isa::pep10::LocalMachine<enable_history>::write_csr(isa::pep10::CSR csr, bool value) {
    return isa::pep10::write_NZVC(*_processor.get(), csr, value);
}

template <bool enable_history> uint8_t isa::pep10::LocalMachine<enable_history>::read_packed_csr() const {
    return isa::pep10::read_packed_NZVC(*_processor.get());
}

template <bool enable_history> void isa::pep10::LocalMachine<enable_history>::write_packed_csr(uint8_t value) {
    return isa::pep10::write_packed_NZVC(*_processor.get(), value);
}

template <bool enable_history> result<void> isa::pep10::LocalMachine<enable_history>::unwind_active_instruction() {
    if constexpr (enable_history) {

        if (auto result_mem = _memory->take_delta(); result_mem.has_error())
            return result_mem.error().clone();
        else
            result_mem.value()->apply_backward();

        if (auto result_reg = _processor->take_register_delta(); result_reg.has_error())
            return result_reg.error().clone();
        else
            result_reg.value()->apply_backward();

        if (auto result_csr = _processor->take_csr_delta(); result_csr.has_error())
            return result_csr.error().clone();
        else
            result_csr.value()->apply_backward();

        return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
    } else {
        return status_code(StorageErrc::DeltaDisabled);
    }
}

template <bool enable_history>
uint16_t isa::pep10::LocalMachine<enable_history>::address_from_vector(isa::pep10::MemoryVector vector) const {
    uint16_t base = _memory->max_offset();
    switch (vector) {
    case isa::pep10::MemoryVector::kUser_Stack:
        return base - 11;
    case isa::pep10::MemoryVector::kSystem_Stack:
        return base - 9;
    case isa::pep10::MemoryVector::kPower_Off_Port:
        return base - 7;
    case isa::pep10::MemoryVector::kDispatcher:
        return base - 5;
    case isa::pep10::MemoryVector::kLoader:
        return base - 3;
    case isa::pep10::MemoryVector::kTrap_Handler:
        return base - 1;
    }
    throw std::logic_error("Invalid memory vector value.");
}

template <bool enable_history>
void isa::pep10::LocalMachine<enable_history>::clear_all(uint8_t mem_fill, uint16_t reg_fill, bool csr_fill) {
    clear_memory(mem_fill);
    clear_processor(reg_fill, csr_fill);
    if constexpr (enable_history)
        clear_deltas().value();
}

template <bool enable_history> void isa::pep10::LocalMachine<enable_history>::clear_memory(uint8_t mem_fill) {
    _memory->clear(mem_fill);
}

template <bool enable_history>
void isa::pep10::LocalMachine<enable_history>::clear_processor(uint16_t reg_fill, bool csr_fill) {
    _processor->clear(reg_fill, csr_fill);
}

template <bool enable_history> void isa::pep10::LocalMachine<enable_history>::add_breakpoint(uint16_t address) {
    _processor->add_breakpoint(address);
}

template <bool enable_history> bool isa::pep10::LocalMachine<enable_history>::remove_breakpoint(uint16_t address) {
    return _processor->remove_breakpoint(address);
}

template <bool enable_history> void isa::pep10::LocalMachine<enable_history>::remove_all_breakpoints() {
    _processor->remove_all_breakpoints();
}

template <bool enable_history> uint64_t isa::pep10::LocalMachine<enable_history>::cycle_count() const {
    return _processor->cycle_count();
}

template <bool enable_history> result<void> isa::pep10::LocalMachine<enable_history>::save_deltas() {
    using _StepDelta = components::machine::StepDelta<uint16_t, uint8_t, uint8_t, uint16_t, uint8_t, bool>;
    if constexpr (enable_history) {
        auto mem = _memory->take_delta();
        if (mem.has_error())
            return mem.error().clone();
        auto reg = _processor->take_register_delta();
        if (reg.has_error())
            return reg.error().clone();
        auto csr = _processor->take_csr_delta();
        if (csr.has_error())
            return csr.error().clone();
        auto merged = _StepDelta(std::move(mem.value()), std::move(reg.value()), std::move(csr.value()));
        _deltas.emplace(cycle_count(), std::move(merged));
        return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
    } else {
        return status_code(StorageErrc::DeltaDisabled);
    }
}

template <bool enable_history> result<void> isa::pep10::LocalMachine<enable_history>::clear_deltas() {
    if constexpr (enable_history) {
        _deltas.clear();
        return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
    } else {
        return status_code(StorageErrc::DeltaDisabled);
    }
}

template <bool enable_history>
void *isa::pep10::LocalMachine<enable_history>::deltas_between(uint64_t start, uint64_t end) const {
    throw std::logic_error("Not implemented");
}

template <bool enable_history> void isa::pep10::LocalMachine<enable_history>::clear_MMIO_addresses() {
    _mmio_mapping.clear();
}

template <bool enable_history>
void isa::pep10::LocalMachine<enable_history>::register_MMIO_address(const std::string &device_name, uint16_t address) {
    _mmio_mapping[device_name] = address;
}

template <bool enable_history>
result<uint16_t> isa::pep10::LocalMachine<enable_history>::device_address(const std::string &device_name) const {
    if (auto addr = _mmio_mapping.find(device_name); addr == _mmio_mapping.end())
        return status_code(MachineErrc::NoSuchDevice);
    else
        return addr->second;
}

template <bool enable_history>
result<components::storage::Input<uint16_t, enable_history, uint8_t> *>
isa::pep10::LocalMachine<enable_history>::input_device(const std::string &device_name) {
    using input_t = components::storage::Input<uint16_t, enable_history, uint8_t> *;
    if (auto addr = _mmio_mapping.find(device_name); addr == _mmio_mapping.end())
        return status_code(MachineErrc::NoSuchDevice);
    else if (auto device = _memory->device_at(addr->second); device.has_error())
        return device.error().clone();
    else if (auto as_input = dynamic_cast<input_t>(device.value()); as_input)
        return as_input;
    return status_code(MachineErrc::NoSuchDevice);
}

template <bool enable_history>
result<components::storage::Output<uint16_t, enable_history, uint8_t> *>
isa::pep10::LocalMachine<enable_history>::output_device(const std::string &device_name) {
    using output_t = components::storage::Output<uint16_t, enable_history, uint8_t> *;
    if (auto addr = _mmio_mapping.find(device_name); addr == _mmio_mapping.end())
        return status_code(MachineErrc::NoSuchDevice);
    else if (auto device = _memory->device_at(addr->second); device.has_error())
        return device.error().clone();
    else if (auto as_input = dynamic_cast<output_t>(device.value()); as_input)
        return as_input;
    return status_code(MachineErrc::NoSuchDevice);
}
template <bool enable_history> void isa::pep10::LocalMachine<enable_history>::begin_transaction() {
    // TODO: Inform memory of transaction start.
    // Only necessary for cache model.
}

template <bool enable_history> void isa::pep10::LocalMachine<enable_history>::end_transaction() {
    // TODO: Inform memory of transaction end.
    // Only necessary for cache model.
}

template <bool enable_history> void isa::pep10::LocalMachine<enable_history>::begin_instruction() {
    // TODO: Pre-fetch the next instruction, and add statstic to histogram.
}

template <bool enable_history> void isa::pep10::LocalMachine<enable_history>::end_instruction() {
    // TODO: Push all deltas onto the history stack.
}

template <bool enable_history>
result<void> isa::pep10::load_bytes(std::shared_ptr<isa::pep10::LocalMachine<enable_history>> machine,
                                    const std::vector<uint8_t> &bytes, uint16_t offset) {
    // Must add 1 to modulus, otherwise we won't be able to "hit" the maximum address.
    assert(machine->max_offset() != 0xFFFF'FFFF'FFFF'FFFF);
    const uint64_t modulo = machine->max_offset() + 1;

    for (auto [index, byte] : bytes | boost::adaptors::indexed(0)) {
        auto res = machine->set_memory((offset + index) % modulo, byte);
        if (res.has_failure())
            return res.error().clone();
    }
    return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
}

// Run the machine forever, or until a value is written to the pwrOff port.
template <bool enable_history> result<void> isa::pep10::run(std::shared_ptr<LocalMachine<enable_history>> machine) {
    result<bool> res = true;
    do {
        res = std::move(machine->step());
        if (res.has_error())
            return res.error().clone();
        // TODO: Add "break" for breakpoints.
    } while (res.value());
    return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
}

// Run the machine forever, or until a value is written to the pwrOff port, or a maximum number of steps is exceeded.
template <bool enable_history>
result<step::Result> isa::pep10::run(std::shared_ptr<LocalMachine<enable_history>> machine, uint64_t max_timesteps) {
    result<step::Result> res = step::Result::kHalted;
    do {
        res = std::move(machine->step());
        if (res.has_error())
            return res.error().clone();
        if (machine->cycle_count() >= max_timesteps)
            break;
        // TODO: Add "break" for breakpoints.
    } while (res.value() == step::Result::kNominal);
    return res;
}