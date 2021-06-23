#include "listing.hpp"

template <typename addr_size_t>
std::string masm::utils::generate_listing(std::shared_ptr<masm::elf::top_level_section<addr_size_t> >& image)
{
	std::stringstream out_stream;
	for(const auto& line : image->body_ir->ir_lines){
		out_stream << line->generate_listing_string() << std::endl;
	}
	return out_stream.str();
}

template <typename addr_size_t>
std::vector<uint8_t> masm::utils::get_bytecode(std::shared_ptr<masm::elf::top_level_section<addr_size_t> >& image)
{
	return {};
}

template <typename addr_size_t>
std::string masm::utils::generate_formatted_bytecode(std::shared_ptr<masm::elf::top_level_section<addr_size_t> >& image)
{
	return {};
}

