#pragma once

#include "masm/ir/base.hpp"
#include "masm/project/project.hpp"
#include "masm/project/section.hpp"
#include "masm/ir/args.hpp"
#include "masm/registry.hpp"

namespace masm::backend
{
	
	// Each image in the project must have its own control script, as address assignment only concerns a single image.
	template <typename addr_size_t>
	auto assign_image(std::shared_ptr<masm::project::project<addr_size_t> >& project, 
		std::shared_ptr<masm::elf::image<addr_size_t> >& image
	) -> bool;

	template <typename addr_size_t>
	auto assign_section_line_numbers(std::shared_ptr<masm::project::project<addr_size_t> >& project, 
		std::shared_ptr<masm::elf::image<addr_size_t> >& image,
		std::shared_ptr<masm::elf::code_section<addr_size_t> >& section,
		addr_size_t& listing_line
	) -> bool;

	template <typename addr_size_t>
	auto assign_section_forward(std::shared_ptr<masm::project::project<addr_size_t> >& project, 
		std::shared_ptr<masm::elf::image<addr_size_t> >& image,
		std::shared_ptr<masm::elf::code_section<addr_size_t> >& section,
		addr_size_t& base_address
	) -> bool;

	template <typename addr_size_t>
	auto assign_section_backward(std::shared_ptr<masm::project::project<addr_size_t> >& project, 
		std::shared_ptr<masm::elf::image<addr_size_t> >& image,
		std::shared_ptr<masm::elf::code_section<addr_size_t> >& section,
		addr_size_t& base_address
	) -> bool;

} // End namespace masm::backend

#include "assign_addr.tpp"