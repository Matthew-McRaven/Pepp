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
	std::shared_ptr<masm::elf::image<addr_size_t> >& image
) -> bool
{
	using tls_ptr_t = std::shared_ptr<masm::elf::top_level_section<addr_size_t> >;
	std::vector<tls_ptr_t> sections = {image->os};
	if(image->user) sections.emplace_back(image->user);
	bool success = true;
	for(auto& section : sections) {
		auto as_code = std::static_pointer_cast<masm::elf::code_section<addr_size_t>>(section);

		if(section->body_ir->BURN_address) {
			auto start_address = section->body_ir->BURN_address.value();
			success &= assign_section_backward(project, image, as_code, start_address);
		}
		else {
			auto start_address = uint16_t{0};
			success &= assign_section_forward(project, image, as_code, start_address);
		}
		address_size_t listing_line = 0
		success &= assign_section_line_numbers(project, image, section, listing_line);
	}
	
	return success;
}

template <typename addr_size_t>
auto masm::backend::assign_section_line_numbers(std::shared_ptr<masm::project::project<addr_size_t> >& project, 
	std::shared_ptr<masm::elf::image<addr_size_t> >& image,
	std::shared_ptr<masm::elf::code_section<addr_size_t> >& section,
	addr_size_t& listing_line
) -> bool
{
	addr_size_t source_line = 0
	auto success = true;
	auto &ir = section->body_ir->ir_lines;
	for(auto& line : ir) {
		line->source_line = source_line;
		line->listing_line = listing_line;
		// Recurse into macro modules.
		if(auto as_macro = std::dynamic_pointer_cast<masm::ir::macro_invocation<addr_size_t>>(line); as_macro) {
			auto as_code = std::static_pointer_cast<masm::elf::code_section<addr_size_t>>(as_macro->macro);
			success &= assign_section_line_numbers(project, image, as_code, listing_line_number);
		} else listing_line += 1;
		source_line += 1;
	}
	return success;
}

template <typename addr_size_t>
auto masm::backend::assign_section_forward(std::shared_ptr<masm::project::project<addr_size_t> >& project, 
	std::shared_ptr<masm::elf::image<addr_size_t> >& image,
	std::shared_ptr<masm::elf::code_section<addr_size_t> >& section,
	addr_size_t& base_address
) -> bool
{
	auto success = true;
	auto &ir = section->body_ir->ir_lines;
	for(auto& line : ir) {
		line->set_begin_address(base_address);
		
		// If an operand is undefined, raise an error.
		if(auto op = line->symbolic_operand(); op && *op) {
			if((*op)->state == symbol::definition_state::kUndefined) {
				success = false;
				project->message_resolver->log_message(section, line->source_line, 
					{masm::message_type::kError, fmt::format(";Error: Undefined symbolic operand \"{}\".", (*op)->name)}
				);
			}
		}

		// If a symbol is singly defined, assign in an address
		if(line->symbol_entry && line->symbol_entry->state == symbol::definition_state::kSingle){
			line->symbol_entry->value = std::make_shared<symbol::value_location<addr_size_t>>(line->base_address(), 0);
		}
		// If a symbol is multiplt defined or externally multiply defined, raise an error
		else if(line->symbol_entry && line->symbol_entry->state == symbol::definition_state::kMultiple){
			success = false;
			project->message_resolver->log_message(section, line->source_line, 
				{masm::message_type::kError, fmt::format(";Error: Multiply defined symbol \"{}\".", line->symbol_entry->name)}
			);
		}
		else if(line->symbol_entry && line->symbol_entry->state == symbol::definition_state::kExternalMultiple){
			success = false;
			project->message_resolver->log_message(section, line->source_line, 
				{masm::message_type::kError, fmt::format(";Error: Conflicting definition of \"{}\" with .EXPORT.", line->symbol_entry->name)}
			);
		}

		// Ensure that alignment directives (e.g., .ALIGN 2) are the proper direction
		if(auto as_align = std::dynamic_pointer_cast<masm::ir::dot_align<addr_size_t>>(line); as_align) {
			as_align->direction = masm::ir::dot_align<addr_size_t>::align_direction::kNext;
		}

		// Recurse into macro modules.
		if(auto as_macro = std::dynamic_pointer_cast<masm::ir::macro_invocation<addr_size_t>>(line); as_macro) {
			auto as_code = std::static_pointer_cast<masm::elf::code_section<addr_size_t>>(as_macro->macro);
			success &= assign_section_forward(project, image, as_code, base_address);
		} 
		 // Don't increment the base address for macros, since it will be incremented in recursive call.
		else {
			// Ensure that incrementing the base address won't cause our addr_size_t to overflow.
			static const uint64_t max_addr = (1 << (8*sizeof(addr_size_t))) - 1;
			bool address_overflow = max_addr - base_address < line->object_code_bytes();
			if(address_overflow) { // Log error with message resolver.
				success = false;
				project->message_resolver->log_message(section, line->source_line, 
					{masm::message_type::kError, ";Error: Positive address overflow."}
				);
				break;
			}
			base_address += line->object_code_bytes();
		}
	}
	return success;
}

template <typename addr_size_t>
auto masm::backend::assign_section_backward(std::shared_ptr<masm::project::project<addr_size_t> >& project, 
	std::shared_ptr<masm::elf::image<addr_size_t> >& image,
	std::shared_ptr<masm::elf::code_section<addr_size_t> >& section,
	addr_size_t& base_address
) -> bool
{
	auto success = true;
	auto &ir = section->body_ir->ir_lines;
	for(auto& line : boost::adaptors::reverse(ir)) {
		line->set_end_address(base_address);

		// If an operand is undefined, raise an error.
		if(auto op = line->symbolic_operand(); op && *op) {
			if((*op)->state == symbol::definition_state::kUndefined) {
				success = false;
				project->message_resolver->log_message(section, line->source_line, 
					{masm::message_type::kError, fmt::format(";Error: Undefined symbolic operand \"{}\".", (*op)->name)}
				);
			}
		}

		// If a symbol is singly defined, assign in an address
		if(line->symbol_entry && line->symbol_entry->state == symbol::definition_state::kSingle){
			line->symbol_entry->value = std::make_shared<symbol::value_location<addr_size_t>>(line->base_address(), 0);
		}
		// If a symbol is multiplt defined or externally multiply defined, raise an error
		else if(line->symbol_entry && line->symbol_entry->state == symbol::definition_state::kMultiple){
			success = false;
			project->message_resolver->log_message(section, line->source_line, 
				{masm::message_type::kError, fmt::format(";Error: Multiply defined symbol \"{}\".", line->symbol_entry->name)}
			);
		}
		else if(line->symbol_entry && line->symbol_entry->state == symbol::definition_state::kExternalMultiple){
			success = false;
			project->message_resolver->log_message(section, line->source_line, 
				{masm::message_type::kError, fmt::format(";Error: Conflicting definition of \"{}\" with .EXPORT.", line->symbol_entry->name)}
			);
		}

		// Recurse into macro modules.
		if(auto as_macro = std::dynamic_pointer_cast<masm::ir::macro_invocation<addr_size_t>>(line); as_macro) {
			auto as_code = std::static_pointer_cast<masm::elf::code_section<addr_size_t>>(as_macro->macro);
			success &= assign_section_backward(project, image, as_code, base_address);;
		} 
		 // Don't increment the base address for macros, since it will be incremented in recursive call.
		else {
			// Ensure that decrementing the base address won't cause our addr_size_t to experience negative overflow.
			bool address_overflow =   base_address + 1< line->object_code_bytes();
			if(address_overflow) { // Log error with message resolver.
				success = false;
				project->message_resolver->log_message(section, line->source_line, 
					{masm::message_type::kError, ";Error: Negative address overflow."}
				);
				break;
			}
			base_address -= line->object_code_bytes();
		}
	}
	return success;
}