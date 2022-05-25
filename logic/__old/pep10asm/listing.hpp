#pragma once

#include <memory>
#include <stdint.h>
#include <vector>

#include "masm/project/section.hpp"

namespace masm::utils {

template <typename addr_size_t>
std::string generate_listing(std::shared_ptr<masm::elf::top_level_section<addr_size_t>> &image);

template <typename addr_size_t>
std::vector<uint8_t> get_bytecode(std::shared_ptr<masm::elf::top_level_section<addr_size_t>> &image);

template <typename addr_size_t>
std::string generate_formatted_bytecode(std::shared_ptr<masm::elf::top_level_section<addr_size_t>> &image,
                                        uint8_t bytes_per_line = 16);

// Use with base=2 to generate pepb, base=16 to generate peph.
// Bases outside of 2,8,16 are allowed to causes runtime error.
template <typename addr_size_t>
std::string generate_pretty_object_code(std::shared_ptr<masm::elf::top_level_section<addr_size_t>> &image,
                                        uint8_t base = 2, bool include_comment = true);
} // namespace masm::utils
#include "listing.tpp"