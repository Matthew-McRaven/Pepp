#include "assign_addr.hpp"
#include "masm/project/image.hpp"
#include "masm/ir/directives.hpp"
#include "masm/ir/macro.hpp"

#include "symbol/entry.hpp"
#include "symbol/value.hpp"


#include <boost/range/adaptor/reversed.hpp>

// Each image in the project must have its own control script.
template <typename addr_size_t>
auto masm::backend::assign_image(std::shared_ptr<masm::project::project<addr_size_t> >& project, 
	std::shared_ptr<masm::elf::image<addr_size_t> >& image,
	std::list<masm::backend::region<addr_size_t> > control_script
) -> bool
{
	using tls_ptr_t = std::shared_ptr<masm::elf::top_level_section<addr_size_t> >;

	// Keep track of which sections *have not* been matched by our control script.
	std::list<tls_ptr_t> unmatched_sections;
	std::copy(image->sections.begin(), image->sections.end(), std::back_inserter(unmatched_sections));

	// Track which sections have been matched, as well as the base addresses associated with those sections.
	using sectionized_region_t = std::tuple<masm::backend::region<addr_size_t>, std::list<tls_ptr_t>>;
	std::list<sectionized_region_t> matched_sections;

	// Collat input sections into the correct output sections.
	for(auto& region : control_script) {
		std::list<tls_ptr_t> region_sections;

		for(auto& section : region.input_sections) {
			tls_ptr_t match = nullptr;
			auto matched = std::remove_if(unmatched_sections.begin(), unmatched_sections.end(), [&section](const auto& it) {
				return it->header.name == section;
			});

			if(matched != unmatched_sections.end()) region_sections.emplace_back(*matched);
			// Warn that an output region was unused.
			else {
				auto message = fmt::format(";WARNING: Unused output section \"{}\"", section);
				project->message_resolver->log_message(image->sections[0], 0, {masm::message_type::kWarning, message});
			} 

			// Remove only moves items to end, it doesn't pop items from container.
			unmatched_sections.erase(matched, unmatched_sections.end());
		}
		matched_sections.emplace_back(sectionized_region_t(region, region_sections));
	}

	// Warn that an input section was unused.
	for(auto section : unmatched_sections) {
		auto message = fmt::format(";WARNING: Unused source section \"{}\"", section->header.name);
		project->message_resolver->log_message(section, 0, {masm::message_type::kWarning, message});
	} 

	// Multiple control scripts may end up generating the same binary.
	// Sort the regions by address to make effectively identical scripts assign address in the same order.
	// This is mainly to help debug, but it also provides stability in terms of error message ordering.
	matched_sections.sort([](const auto& lhs, const auto& rhs) {
		return std::get<0>(lhs).base_address < std::get<0>(rhs).base_address;
	});

	bool success = true;
	for(auto& packed_region : matched_sections) {
		auto region = std::get<0>(packed_region);
		auto output_sections = std::get<1>(packed_region);
		addr_size_t base_address = region.base_address;

		if(region.direction == masm::backend::align_direction::TOP) {
			for(auto& section : output_sections) {
				base_address += ((region.alignment) - (base_address % (region.alignment))) % (region.alignment);
				auto as_code = std::static_pointer_cast<masm::elf::code_section<addr_size_t>>(section);
				// Ensure the current section starts on an alignment boundary.
				success &= assign_section_top(project, image, as_code, base_address);
				
			}
		}
		// Must start from last section and assign addresses from bottom to top.
		else if(region.direction == masm::backend::align_direction::BOTTOM) {
			for(auto& section : boost::adaptors::reverse(output_sections)) {
				auto as_code = std::static_pointer_cast<masm::elf::code_section<addr_size_t>>(section);
				success &= assign_section_bottom(project, image, as_code, base_address);
				// Ensure the current section starts on an alignment boundary.s
				base_address -= (base_address % (region.alignment));
			}
		}
		
	}

	return success;

}

template <typename addr_size_t>
auto masm::backend::assign_section_top(std::shared_ptr<masm::project::project<addr_size_t> >& project, 
	std::shared_ptr<masm::elf::image<addr_size_t> >& image,
	std::shared_ptr<masm::elf::code_section<addr_size_t> >& section,
	addr_size_t& base_address
) -> bool
{
	auto success = true;
	auto &ir = section->body_ir->ir_lines;
	for(auto& line : ir) {
		line->set_begin_address(base_address);
		if(line->symbol_entry){
			line->symbol_entry->value = std::make_shared<symbol::value_location<addr_size_t>>(line->base_address(), 0);
		}

		// Ensure that alignment directives (e.g., .ALIGN 2) are the proper direction
		if(auto as_align = std::dynamic_pointer_cast<masm::ir::dot_align<addr_size_t>>(line); as_align) {
			as_align->direction = masm::ir::dot_align<addr_size_t>::align_direction::kNext;
		}

		// Recurse into macro modules.
		if(auto as_macro = std::dynamic_pointer_cast<masm::ir::macro_invocation<addr_size_t>>(line); as_macro) {
			auto as_code = std::static_pointer_cast<masm::elf::code_section<addr_size_t>>(as_macro->macro);
			success &= assign_section_top(project, image, as_code, base_address);
		} 
		 // Don't increment the base address for macros, since it will be incremented in recursive call.
		else {
			// Ensure that incrementing the base address won't cause our addr_size_t to overflow.
			static const uint64_t max_addr = (1 << (8*sizeof(addr_size_t))) - 1;
			success &=  max_addr - base_address >= line->object_code_bytes();
			if(!success) { // TODO: Log error with message resolver.
				break;
			}
			base_address += line->object_code_bytes();
		}
	}
	return success;
}

template <typename addr_size_t>
auto masm::backend::assign_section_bottom(std::shared_ptr<masm::project::project<addr_size_t> >& project, 
	std::shared_ptr<masm::elf::image<addr_size_t> >& image,
	std::shared_ptr<masm::elf::code_section<addr_size_t> >& section,
	addr_size_t& base_address
) -> bool
{
	auto success = true;
	auto &ir = section->body_ir->ir_lines;
	for(auto& line : boost::adaptors::reverse(ir)) {
		// Ensure that alignment directives (e.g., .ALIGN 2) are the proper direction
		if(auto as_align = std::dynamic_pointer_cast<masm::ir::dot_align<addr_size_t>>(line); as_align) {
			as_align->direction = masm::ir::dot_align<addr_size_t>::align_direction::kPrevious;
		}

		line->set_end_address(base_address);
		if(line->symbol_entry){
			line->symbol_entry->value = std::make_shared<symbol::value_location<addr_size_t>>(line->base_address(), 0);
		};

		// Recurse into macro modules.
		if(auto as_macro = std::dynamic_pointer_cast<masm::ir::macro_invocation<addr_size_t>>(line); as_macro) {
			auto as_code = std::static_pointer_cast<masm::elf::code_section<addr_size_t>>(as_macro->macro);
			success &= assign_section_top(project, image, as_code, base_address);;
		} 
		 // Don't increment the base address for macros, since it will be incremented in recursive call.
		else {
			// Ensure that decrementing the base address won't cause our addr_size_t to experience negative overflow.
			success &=   base_address >= line->object_code_bytes();
			if(!success) { // TODO: Log error with message resolver.
				break;
			}
			base_address -= line->object_code_bytes();
		}
	}
	return success;
}