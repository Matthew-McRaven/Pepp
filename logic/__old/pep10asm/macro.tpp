#include "macro.hpp"
#include <boost/algorithm/string.hpp>

template <typename address_size_t>
masm::ir::macro_invocation<address_size_t>::macro_invocation()
{
	this->emits_object_code = true;
}
template <typename address_size_t>
masm::ir::macro_invocation<address_size_t>::macro_invocation(
	const masm::ir::macro_invocation<address_size_t>& other)
{

}

template <typename address_size_t>
masm::ir::macro_invocation<address_size_t> &masm::ir::macro_invocation<address_size_t>::operator=(
	masm::ir::macro_invocation<address_size_t> other)
{
    swap(*this, other);
    return *this;
}


template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::macro_invocation<address_size_t>::clone() const
{
    return std::make_shared<macro_invocation<address_size_t> >(*this);
}

template <typename address_size_t>
std::string masm::ir::macro_invocation<address_size_t>::generate_listing_string() const
{
	auto temp = fmt::format("{:<6} {:<6};@{} {}",
		""
		"",
		macro->header.name,
		boost::algorithm::join(macro->macro_args, ", ")
	);
	
	for(auto line : macro->body_ir.value().ir_lines)
	{
		// TODO: Skip line if it is a .END
		temp.append(fmt::format("\n{}}", line->generate_listing_string()));
	}
	temp.append(fmt::format("\n;End @{}", macro->header.name));
	if(this->comment) {
		// TODO: Figure out where to place comments in listing!!
	}
	return temp;
}

template <typename address_size_t>
std::string masm::ir::macro_invocation<address_size_t>::generate_source_string() const
{
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
	auto macro_name = macro->header.name;
	auto operand_string = boost::algorithm::join(macro->macro_args, ", ");
	std::string comment = this->comment.value_or("");
    return fmt::format("{:<9}{:<8}{:<12}{}",
		symbol_string,
		macro_name,
		operand_string,
		comment
	);

}

template <typename address_size_t>
address_size_t masm::ir::macro_invocation<address_size_t>::object_code_bytes() const
{
	address_size_t bytes = 0;
	for(auto line : macro->body_ir.value().ir_lines)
	{
		bytes += line->object_code_bytes();
	}
	return bytes;
}

template <typename address_size_t>
void masm::ir::macro_invocation<address_size_t>::append_object_code(std::vector<uint8_t>& bytes) const
{
	if(!this->emits_object_code) return;
	for(const auto& line: macro->body_ir.value().ir_lines) {
		line->append_object_code(bytes);
	}
}
