#pragma once

#include <memory>
#include <stdint.h>
#include <vector>

#include "masm/project/section.hpp"

namespace masm::utils
{

	template <typename addr_size_t>
	std::string generate_listing(std::shared_ptr<masm::elf::top_level_section<addr_size_t> >& image);

	template <typename addr_size_t>
	std::vector<uint8_t> get_bytecode(std::shared_ptr<masm::elf::top_level_section<addr_size_t> >& image);

	template <typename addr_size_t>
	std::string generate_formatted_bytecode(std::shared_ptr<masm::elf::top_level_section<addr_size_t> >& image);

}
#include "listing.tpp"