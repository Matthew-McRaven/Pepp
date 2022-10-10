#pragma once

#include <cstdint>
#include <memory>

#include <elfio/elfio.hpp>
#include <outcome.hpp>

#include "components/storage/block.hpp"
#include "components/storage/layered.hpp"
#include "emu/local_machine.hpp"
#include "asmdr/elf/mmio.hpp"

namespace elf_tools::pep10 {

// If possible, create a (read-only) storage device large enough to contain the contents of the os.text section.
template <bool enable_history>
result<std::tuple<uint16_t, std::shared_ptr<components::storage::Base<uint16_t, enable_history, uint8_t>>>>
construct_os_storage(const ELFIO::elfio &image);

// Fill OS ROM with the contents of the os.text section.
template <bool enable_history>
result<void> load_os_contents(const ELFIO::elfio &image,
                              std::shared_ptr<components::storage::Base<uint16_t, enable_history, uint8_t>> storage);

// Extract the names, addresses, and types of all memory-mapped IO ports in the elf binary.
struct PortDefinition {
    uint16_t offset;
    masm::elf::mmio::Type type;
    std::string name;
};

result<std::vector<PortDefinition>> port_definitions(const ELFIO::elfio &image);

// Create all memory-mapped ports for ELF definition.
template <bool enable_history>
result<std::vector<std::tuple<uint16_t, std::shared_ptr<components::storage::Base<uint16_t, enable_history, uint8_t>>>>>
construct_mmio_ports(const ELFIO::elfio &image);

// Create a RAM chip that spans remaining address space
template <bool enable_history>
result<std::tuple<uint16_t, std::shared_ptr<components::storage::Base<uint16_t, enable_history, uint8_t>>>>
construct_ram(const ELFIO::elfio &image);

// Construct all portions of the Pep/10 memory map, and stick them in a Layered storage device.
// Layered storage device will first contain IO ports, the OS ROM, then all RAM.
template <bool enable_history>
result<std::shared_ptr<components::storage::Layered<uint16_t, enable_history, uint8_t>>>
construct_memory_map(const ELFIO::elfio &image);

} // end namespace elf_tools::pep10

#include "mem_map.tpp"
