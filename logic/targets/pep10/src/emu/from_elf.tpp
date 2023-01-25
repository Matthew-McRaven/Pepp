#include "local_machine.hpp"

#include "utils/elf_helper.hpp"
#include "elf/mem_map.hpp"
#include "asmdr/utils/listing.hpp"
#include "utils/conversion.hpp"

// Run the machine forever, or until a value is written to the pwrOff port, or a maximum number of steps is exceeded.
template <bool enable_history>
result<std::shared_ptr<isa::pep10::LocalMachine<enable_history>>>
isa::pep10::machine_from_elf(const ELFIO::elfio &image) {
    auto storage = elf_tools::pep10::construct_memory_map<enable_history>(image);
    if (storage.has_error())
        return storage.error().clone();
    auto machine = std::make_shared<isa::pep10::LocalMachine<enable_history>>(storage.value());

    // Register MMIO ports with the machine.
    auto ports = elf_tools::pep10::port_definitions(image);
    if (ports.has_error())
        return ports.error().clone();
    for (auto port : ports.value()) {
        machine->register_MMIO_address(port.name, port.offset);
    }

    return machine;
}

// Either load the user program into RAM or buffer it behind disk in.
template <bool enable_history>
result<void> isa::pep10::load_user_program(const ELFIO::elfio &image,
                                           std::shared_ptr<isa::pep10::LocalMachine<enable_history>> machine,
                                           isa::pep10::Loader loader_policy) {
    auto bytes_result = elf_tools::section_as_bytes(image, "user.text");
    if (bytes_result.has_error())
        return bytes_result.error().clone();
    auto bytes = bytes_result.value();

    if (loader_policy == isa::pep10::Loader::kDiskIn) {
        auto diskIn_result = machine->input_device("diskIn");
        if (diskIn_result.has_error())
            return diskIn_result.error().clone();
        auto diskIn = diskIn_result.value();
        std::string bytes_as_hex = utils::bytes_to_hex_string(bytes, 16, true);
        // Reinterpret char* as uint8_t, make a copy along the way.
        bytes = std::vector<uint8_t>(bytes_as_hex.begin(), bytes_as_hex.end());
        components::storage::buffer_input(*diskIn, bytes);
        return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
    } else if (loader_policy == isa::pep10::Loader::kRAM) {
        return isa::pep10::load_bytes(machine, bytes, 0);
    } else {
        return system_error2::errc::invalid_argument;
    }
}
