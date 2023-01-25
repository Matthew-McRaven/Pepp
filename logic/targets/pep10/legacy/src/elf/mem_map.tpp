#include <elfio/elfio.hpp>

#include "components/storage/base.hpp"
#include "components/storage/block.hpp"
#include "components/storage/layered.hpp"
#include "components/storage/mmio.hpp"
#include "components/storage/read_only.hpp"
#include "utils/elf_helper.hpp"
#include "pepx/pep_elf_error.hpp"

template <bool enable_history>
result<std::tuple<uint16_t, std::shared_ptr<components::storage::Base<uint16_t, enable_history, uint8_t>>>>
elf_tools::pep10::construct_os_storage(const ELFIO::elfio &image) {
    auto os = elf_tools::find_section(image, "os.text");
    if (os == nullptr)
        return status_code(PepElfErrc::NoOSText);
    // Must convert size (1-indexed) to maximum offset (0-indexed), so must subtract 1.
    auto os_ram = std::make_shared<components::storage::Block<uint16_t, enable_history, uint8_t>>(os->get_size() - 1);
    auto os_rom = std::make_shared<components::storage::ReadOnly<uint16_t, enable_history, uint8_t>>(
        os_ram, components::storage::WriteAttemptPolicy::kIgnore);
    return {os->get_address(), os_rom};
}

template <bool enable_history>
result<void> elf_tools::pep10::load_os_contents(
    const ELFIO::elfio &image, std::shared_ptr<components::storage::Base<uint16_t, enable_history, uint8_t>> storage) {
    auto os = elf_tools::section_as_bytes(image, "os.text");
    if (os.has_error())
        return os.error().clone();
    auto bytes = os.value();
    auto load_result = components::storage::load_bytes<uint16_t, enable_history, uint8_t>(*storage, bytes, 0);
    if (load_result.has_error())
        return load_result.error().clone();
    return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
}

template <bool enable_history>
result<std::tuple<uint16_t, std::shared_ptr<components::storage::Base<uint16_t, enable_history, uint8_t>>>>
elf_tools::pep10::construct_ram(const ELFIO::elfio &image) {
    auto os = elf_tools::find_section(image, "os.text");
    if (os == nullptr)
        return status_code(PepElfErrc::NoOSText);
    // RAM will span from [0, start of os). Layered storage will prefer OS to RAM, however.
    // Must convert size (1-indexed) to maximum offset (0-indexed), so must subtract 1.
    auto ram = std::make_shared<components::storage::Block<uint16_t, enable_history, uint8_t>>(os->get_address() - 1);
    return {0, ram};
}

// Create all memory-mapped ports for ELF definition.
template <bool enable_history>
result<std::vector<std::tuple<uint16_t, std::shared_ptr<components::storage::Base<uint16_t, enable_history, uint8_t>>>>>
elf_tools::pep10::construct_mmio_ports(const ELFIO::elfio &image) {
    auto ports = port_definitions(image);
    if (ports.has_error())
        return ports.error().clone();
    auto ret = std::vector<
        std::tuple<uint16_t, std::shared_ptr<components::storage::Base<uint16_t, enable_history, uint8_t>>>>({});
    for (auto port : ports.value()) {
        std::shared_ptr<components::storage::Base<uint16_t, enable_history, uint8_t>> storage;
        if (port.type == masm::elf::mmio::Type::kInput)
            storage = std::make_shared<components::storage::Input<uint16_t, enable_history, uint8_t>>(0);
        else if (port.type == masm::elf::mmio::Type::kOutput)
            storage = std::make_shared<components::storage::Output<uint16_t, enable_history, uint8_t>>(0);
        else
            return status_code(PepElfErrc::InvalidMMIOType);
        ret.emplace_back(std::make_tuple(port.offset, storage));
    }
    return ret;
}

template <bool enable_history>
result<std::shared_ptr<components::storage::Layered<uint16_t, enable_history, uint8_t>>>
elf_tools::pep10::construct_memory_map(const ELFIO::elfio &image) {
    // Create OS storage and load OS bytes.
    auto ROM_result = construct_os_storage<enable_history>(image);
    if (ROM_result.has_error())
        ROM_result.error().clone();
    auto [ROM_offset, ROM_storage] = ROM_result.value();
    auto load_os = load_os_contents(image, ROM_storage);
    if (load_os.has_error())
        return load_os.error().clone();

    // Create RAM storage and DO NOT load user bytes.
    auto RAM_result = construct_ram<enable_history>(image);
    if (RAM_result.has_error())
        RAM_result.error().clone();
    auto [RAM_offset, RAM_storage] = RAM_result.value();

    // Construct MMIO ports
    auto ports = construct_mmio_ports<enable_history>(image);

    // Construct memory map for components
    using layered_t = components::storage::Layered<uint16_t, enable_history, uint8_t>;
    auto layered = std::make_shared<components::storage::Layered<uint16_t, enable_history, uint8_t>>(
        0xFFFF, 0, layered_t::ReadMiss::kOOB, layered_t::WriteMiss::kOOB);
    for (auto [port_offset, port_storage] : ports.value()) {
        auto append_port = layered->append_storage(port_offset, port_storage);
        if (append_port.has_error())
            return append_port.error().clone();
    }

    auto append_ROM = layered->append_storage(ROM_offset, ROM_storage);
    if (append_ROM.has_error())
        return append_ROM.error().clone();

    auto append_RAM = layered->append_storage(RAM_offset, RAM_storage);
    if (append_RAM.has_error())
        return append_RAM.error().clone();

    return layered;
}
