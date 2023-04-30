#pragma once

#include <memory>

#include <elfio/elfio.hpp>
#include <outcome.hpp>

#include "emu/local_machine.hpp"
#include "isa/pep10.hpp"
#include "utils/outcome_helper.hpp"

namespace isa::pep10 {
enum class Loader {
  kDiskIn, // Buffer the user program behind diskIn.
  kRAM,    // Load the user program into RAM starting at address 0x00.
};

// Convert an ELF binary into a working machine configuration
template<bool enable_history>
result<std::shared_ptr<isa::pep10::LocalMachine<enable_history>>> machine_from_elf(const ELFIO::elfio &image);

// Either load the user program into RAM or buffer it behind disk in.
template<bool enable_history>
result<void> load_user_program(const ELFIO::elfio &image,
                               std::shared_ptr<isa::pep10::LocalMachine<enable_history>> machine, Loader loader_policy);

} // end namespace isa::pep10

#include "from_elf.tpp"
