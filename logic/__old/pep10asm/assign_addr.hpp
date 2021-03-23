#pragma once

#include "masm/ir/base.hpp"
#include "masm/project/project.hpp"
#include "masm/project/section.hpp"
#include "masm/ir/args.hpp"
#include "masm/registry.hpp"

namespace masm::backend
{
	enum class align_direction
	{
		TOP,
		BOTTOM
	};

	template <typename addr_size_t>
	struct region 
	{
		addr_size_t base_address = {0};
		align_direction direction = {align_direction::TOP};
		// Converted to "1<<alignment". Determines what address each named section must be aligned to.
		// Alignments other than 0 may lead to gaps between sections, which will be padded with 0's
		// via a .BLOCK directive.
		uint16_t alignment = {1}; 
		
		std::list<std::string> input_sections;
	};

	// Each image in the project must have its own control script, as address assignment only concerns a single image.
	template <typename addr_size_t>
	auto assign_image(std::shared_ptr<masm::project::project<addr_size_t> >& project, 
		std::shared_ptr<masm::elf::image<addr_size_t> >& image,
		std::list<region<addr_size_t> > control_script
	) -> bool;
	
	template <typename addr_size_t>
	auto assign_section_top(std::shared_ptr<masm::project::project<addr_size_t> >& project, 
		std::shared_ptr<masm::elf::image<addr_size_t> >& image,
		std::shared_ptr<masm::elf::code_section<addr_size_t> >& section,
		addr_size_t& base_address
	) -> bool;

	template <typename addr_size_t>
	auto assign_section_bottom(std::shared_ptr<masm::project::project<addr_size_t> >& project, 
		std::shared_ptr<masm::elf::image<addr_size_t> >& image,
		std::shared_ptr<masm::elf::code_section<addr_size_t> >& section,
		addr_size_t& base_address
	) -> bool;

} // End namespace masm::backend

#include "assign_addr.tpp"