#include "directives.hpp"

#include <fmt/core.h>

#include "masm/conversion.hpp"
#include "symbol/table.hpp"

/*
 * .BLOCK
 */
template <typename address_size_t>
masm::ir::dot_address<address_size_t>::dot_address(const masm::ir::dot_address<address_size_t>& other)
{

}

template <typename address_size_t>
masm::ir::dot_address<address_size_t> &masm::ir::dot_address<address_size_t>::operator=(
	masm::ir::dot_address<address_size_t> other)
{
    swap(*this, other);
    return *this;
}


template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_address<address_size_t>::clone() const
{
    return std::make_shared<dot_address<address_size_t> >(*this);
}

template <typename address_size_t>
std::string masm::ir::dot_address<address_size_t>::generate_listing_string() const
{
    // Potentially skip codegen
    std::string code_string = "";
    if(this->emits_object_code) {
		code_string = fmt::format("{:04X}", this->argument->value());
    }

	return fmt::format("{:<6}{:>7}{}",
		fmt::format("0x{:04X}", this->base_address()),
		code_string,
		generate_source_string()
	);
}

template <typename address_size_t>
std::string masm::ir::dot_address<address_size_t>::generate_source_string() const
{
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto dot_string = ".ADDRSS";
    auto operand_string = argument->string();
	std::string comment = this->comment.value_or("");
    return fmt::format("{:<9}{:<8}{:<12}{}",
		symbol_string,
		dot_string,
		operand_string,
		comment
	);

}

template <typename address_size_t>
address_size_t masm::ir::dot_address<address_size_t>::object_code_bytes() const
{
    return 2;
}

template <typename address_size_t>
void masm::ir::dot_address<address_size_t>::append_object_code(std::vector<uint8_t>& bytes) const
{
	bytes.emplace_back((symbol_entry->value() >> 8 ) & 0xff);
	bytes.emplace_back(symbol_entry->value() & 0xff);
}

template <typename address_size_t>
std::optional<std::shared_ptr<const symbol::entry<address_size_t> > > masm::ir::dot_address<address_size_t>::symbolic_operand() const
{
    // The value of a .addrss instruction is always the value of another symbol.
    return argument.get()->symbol_value();
}

/*
 * .ALIGN
 */
template <typename address_size_t>
masm::ir::dot_align<address_size_t>::dot_align()
{
	this->emits_object_code = true;
}
template <typename address_size_t>
masm::ir::dot_align<address_size_t>::dot_align(const masm::ir::dot_align<address_size_t>& other)
{

}

template <typename address_size_t>
masm::ir::dot_align<address_size_t> &masm::ir::dot_align<address_size_t>::operator=(
	masm::ir::dot_align<address_size_t> other)
{
    swap(*this, other);
    return *this;
}


template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_align<address_size_t>::clone() const
{
    return std::make_shared<dot_align<address_size_t> >(*this);
}

template <typename address_size_t>
std::string masm::ir::dot_align<address_size_t>::generate_listing_string() const
{
    // Potentially skip codegen
    std::string code_string = "";
	auto bytes_remaining = this->num_bytes_generated();
	auto bytes_emitted = 0;

    while(this->emits_object_code && (bytes_remaining>0) && (bytes_emitted<3)) {		
		code_string.append("00");
		++bytes_emitted;
		--bytes_remaining;
    }

	auto temp = fmt::format("{:<6} {:<6}{}",
		fmt::format("0x{:04X}", this->base_address()),
		code_string,
		generate_source_string()
	);

	while(bytes_remaining>0) {
		code_string = "";
		bytes_emitted = 0;
		while(this->emits_object_code && (bytes_remaining>0) && (bytes_emitted<3)) {		
			code_string.append("00");
			++bytes_emitted;
			--bytes_remaining;
		}
		
		temp.append(fmt::format("\n        {:<6}", code_string));
	}
	return temp;
}

template <typename address_size_t>
std::string masm::ir::dot_align<address_size_t>::generate_source_string() const
{
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto dot_string = ".ALIGN";
    auto operand_string = argument->string();
	std::string comment = this->comment.value_or("");
    return fmt::format("{:<9}{:<8}{:<12}{}",
		symbol_string,
		dot_string,
		operand_string,
		comment
	);

}

template <typename address_size_t>
void masm::ir::dot_align<address_size_t>::set_begin_address(address_size_t addr)
{
	auto value = argument->value();
	auto span = (value - (addr % value)) % value;
	this->address_span = {addr, addr+span};
}

template <typename address_size_t>
void masm::ir::dot_align<address_size_t>::set_end_address(address_size_t addr)
{
	auto value = argument->value();
	auto span = addr % value;
	this->address_span = {addr-span, addr};
}

template <typename address_size_t>
address_size_t masm::ir::dot_align<address_size_t>::object_code_bytes() const
{
    return this->num_bytes_generated();
}

template <typename address_size_t>
void masm::ir::dot_align<address_size_t>::append_object_code(std::vector<uint8_t>& bytes) const
{
	// TODO: Allow fill values other than 0.
	for(auto it=0; it<this->num_bytes_generated(); it++) bytes.emplace_back(0);
}

template <typename address_size_t>
address_size_t masm::ir::dot_align<address_size_t>::num_bytes_generated() const
{
	
	return std::get<1>(this->address_span) - std::get<0>(this->address_span);
}

/*
 * .ASCII
 */
template <typename address_size_t>
masm::ir::dot_ascii<address_size_t>::dot_ascii()
{
	this->emits_object_code = true;
}
template <typename address_size_t>
masm::ir::dot_ascii<address_size_t>::dot_ascii(const masm::ir::dot_ascii<address_size_t>& other)
{

}

template <typename address_size_t>
masm::ir::dot_ascii<address_size_t> &masm::ir::dot_ascii<address_size_t>::operator=(
	masm::ir::dot_ascii<address_size_t> other)
{
    swap(*this, other);
    return *this;
}


template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_ascii<address_size_t>::clone() const
{
    return std::make_shared<dot_ascii<address_size_t> >(*this);
}

template <typename address_size_t>
std::string masm::ir::dot_ascii<address_size_t>::generate_listing_string() const
{
    // Potentially skip codegen
    std::string code_string = "";
	
	auto bytes_emitted = 0;

	decltype(std::string().substr({},{})) aliased = this->argument->string();
	auto bytes = masm::byte_vector(aliased);
	auto bytes_head = bytes.begin();
    while(this->emits_object_code && (bytes_head!=bytes.end()) && (bytes_emitted<3)) {		
		code_string.append(fmt::format("{:02X}", *bytes_head++));
		++bytes_emitted;
    }

	auto temp = fmt::format("{:<6} {:<6}{}",
		fmt::format("0x{:04X}", this->base_address()),
		code_string,
		generate_source_string()
	);
	
	while(bytes_head!=bytes.end()) {
		code_string = "";
		bytes_emitted = 0;
		while(this->emits_object_code && (bytes_head!=bytes.end()) && (bytes_emitted<3)) {		
			code_string.append(fmt::format("{:02X}", *bytes_head++));
			++bytes_emitted;
		}
		
		temp.append(fmt::format("\n        {:<6}", code_string));
	}
	return temp;
}

template <typename address_size_t>
std::string masm::ir::dot_ascii<address_size_t>::generate_source_string() const
{
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto dot_string = ".ASCII";
    auto operand_string = argument->string();
	std::string comment = this->comment.value_or("");
    return fmt::format("{:<9}{:<8}{:<12}{}",
		symbol_string,
		dot_string,
		"\""+operand_string+"\"",
		comment
	);

}

template <typename address_size_t>
address_size_t masm::ir::dot_ascii<address_size_t>::object_code_bytes() const
{
	decltype(std::string().substr({},{})) aliased = this->argument->string();
    return masm::byte_string_length(aliased);
}

template <typename address_size_t>
void masm::ir::dot_ascii<address_size_t>::append_object_code(std::vector<uint8_t>& bytes) const
{
	for(auto ch : argument->string()) bytes.emplace_back((uint8_t) ch);
}
/*
 * .BLOCK
 */
template <typename address_size_t>
masm::ir::dot_block<address_size_t>::dot_block()
{
	this->emits_object_code = true;
}
template <typename address_size_t>
masm::ir::dot_block<address_size_t>::dot_block(const masm::ir::dot_block<address_size_t>& other)
{

}

template <typename address_size_t>
masm::ir::dot_block<address_size_t> &masm::ir::dot_block<address_size_t>::operator=(
	masm::ir::dot_block<address_size_t> other)
{
    swap(*this, other);
    return *this;
}


template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_block<address_size_t>::clone() const
{
    return std::make_shared<dot_block<address_size_t> >(*this);
}

template <typename address_size_t>
std::string masm::ir::dot_block<address_size_t>::generate_listing_string() const
{
    // Potentially skip codegen
    std::string code_string = "";
	
	auto bytes_emitted = 0;

	auto bytes = std::vector(this->argument->value(), 0);
	auto bytes_head = bytes.begin();
    while(this->emits_object_code && (bytes_head!=bytes.end()) && (bytes_emitted<3)) {		
		code_string.append(fmt::format("{:02X}", *bytes_head++));
		++bytes_emitted;
    }

	auto temp = fmt::format("{:<6} {:<6}{}",
		fmt::format("0x{:04X}", this->base_address()),
		code_string,
		generate_source_string()
	);
	
	while(bytes_head!=bytes.end()) {
		code_string = "";
		bytes_emitted = 0;
		while(this->emits_object_code && (bytes_head!=bytes.end()) && (bytes_emitted<3)) {		
			code_string.append(fmt::format("{:02X}", *bytes_head++));
			++bytes_emitted;
		}
		
		temp.append(fmt::format("\n        {:<6}", code_string));
	}
	return temp;
}

template <typename address_size_t>
std::string masm::ir::dot_block<address_size_t>::generate_source_string() const
{
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto dot_string = ".BLOCK";
    auto operand_string = argument->string();
	std::string comment = this->comment.value_or("");
    return fmt::format("{:<9}{:<8}{:<12}{}",
		symbol_string,
		dot_string,
		operand_string,
		comment
	);

}

template <typename address_size_t>
address_size_t masm::ir::dot_block<address_size_t>::object_code_bytes() const
{
	return this->argument->value();
}

template <typename address_size_t>
void masm::ir::dot_block<address_size_t>::append_object_code(std::vector<uint8_t>& bytes) const
{
	for(auto it=0; it<argument->value(); it++) bytes.emplace_back(0);
}

template <typename address_size_t>
bool masm::ir::dot_block<address_size_t>::tracks_trace_tags() const
{
	return true;
}

/*
 * .BURN
 */
template <typename address_size_t>
masm::ir::dot_burn<address_size_t>::dot_burn(const masm::ir::dot_burn<address_size_t>& other)
{

}

template <typename address_size_t>
masm::ir::dot_burn<address_size_t> &masm::ir::dot_burn<address_size_t>::operator=(
	masm::ir::dot_burn<address_size_t> other)
{
    swap(*this, other);
    return *this;
}


template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_burn<address_size_t>::clone() const
{
    return std::make_shared<dot_burn<address_size_t> >(*this);
}

template <typename address_size_t>
std::string masm::ir::dot_burn<address_size_t>::generate_listing_string() const
{
	auto temp = fmt::format("{:<6} {:<6}{}",
		"",
		"", // Doesn't generate any code!
		generate_source_string()
	);
	
	return temp;
}

template <typename address_size_t>
std::string masm::ir::dot_burn<address_size_t>::generate_source_string() const
{

    auto dot_string = ".BURN";
    auto operand_string = argument->string();
	std::string comment = this->comment.value_or("");
    return fmt::format("{:<9}{:<8}{:<12}{}",
		"",
		dot_string,
		operand_string,
		comment
	);

}

template <typename address_size_t>
void masm::ir::dot_burn<address_size_t>::append_object_code(std::vector<uint8_t>& bytes) const
{
	return;
}

/*
 * .BYTE
 */
template <typename address_size_t>
masm::ir::dot_byte<address_size_t>::dot_byte()
{
	this->emits_object_code = true;
}

template <typename address_size_t>
masm::ir::dot_byte<address_size_t>::dot_byte(const masm::ir::dot_byte<address_size_t>& other)
{

}

template <typename address_size_t>
masm::ir::dot_byte<address_size_t> &masm::ir::dot_byte<address_size_t>::operator=(
	masm::ir::dot_byte<address_size_t> other)
{
    swap(*this, other);
    return *this;
}


template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_byte<address_size_t>::clone() const
{
    return std::make_shared<dot_byte<address_size_t> >(*this);
}

template <typename address_size_t>
std::string masm::ir::dot_byte<address_size_t>::generate_listing_string() const
{
    // Potentially skip codegen
    std::string code_string = "";
	if(this->emits_object_code) {
		code_string = fmt::format("{:02X}", this->argument->value() & 0xff);
	}


	auto temp = fmt::format("{:<6} {:<6}{}",
		fmt::format("0x{:04X}", this->base_address()),
		code_string,
		generate_source_string()
	);
	
	return temp;
}

template <typename address_size_t>
std::string masm::ir::dot_byte<address_size_t>::generate_source_string() const
{
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto dot_string = ".BYTE";
    auto operand_string = argument->string();
	std::string comment = this->comment.value_or("");
    return fmt::format("{:<9}{:<8}{:<12}{}",
		symbol_string,
		dot_string,
		operand_string,
		comment
	);

}

template <typename address_size_t>
address_size_t masm::ir::dot_byte<address_size_t>::object_code_bytes() const
{
	return 1;
}

template <typename address_size_t>
void masm::ir::dot_byte<address_size_t>::append_object_code(std::vector<uint8_t>& bytes) const
{
	bytes.emplace_back(argument->value() & 0xff);;
}

template <typename address_size_t>
bool masm::ir::dot_byte<address_size_t>::tracks_trace_tags() const
{
	return true;
}

/*
 * .END
 */
template <typename address_size_t>
masm::ir::dot_end<address_size_t>::dot_end(const masm::ir::dot_end<address_size_t>& other)
{

}

template <typename address_size_t>
masm::ir::dot_end<address_size_t> &masm::ir::dot_end<address_size_t>::operator=(
	masm::ir::dot_end<address_size_t> other)
{
    swap(*this, other);
    return *this;
}


template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_end<address_size_t>::clone() const
{
    return std::make_shared<dot_end<address_size_t> >(*this);
}

template <typename address_size_t>
std::string masm::ir::dot_end<address_size_t>::generate_listing_string() const
{
	auto temp = fmt::format("{:<6} {:<6}{}",
		"", // Doesn't have an address
		"", // Doesn't generate any code!
		generate_source_string()
	);
	
	return temp;
}

template <typename address_size_t>
std::string masm::ir::dot_end<address_size_t>::generate_source_string() const
{
    auto dot_string = ".END";
	std::string comment = this->comment.value_or("");
    return fmt::format("{:<9}{:<8}{:<12}{}",
		"",
		dot_string,
		"",
		comment
	);

}

template <typename address_size_t>
void masm::ir::dot_end<address_size_t>::append_object_code(std::vector<uint8_t>& bytes) const
{
	return;
}
/*
 * .EQUATE
 */

template <typename address_size_t>
masm::ir::dot_equate<address_size_t>::dot_equate(const masm::ir::dot_equate<address_size_t>& other)
{

}

template <typename address_size_t>
masm::ir::dot_equate<address_size_t> &masm::ir::dot_equate<address_size_t>::operator=(
	masm::ir::dot_equate<address_size_t> other)
{
    swap(*this, other);
    return *this;
}


template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_equate<address_size_t>::clone() const
{
    return std::make_shared<dot_equate<address_size_t> >(*this);
}

template <typename address_size_t>
std::string masm::ir::dot_equate<address_size_t>::generate_listing_string() const
{
	auto temp = fmt::format("{:<6} {:<6}{}",
		"",
		"",
		generate_source_string()
	);
	
	return temp;
}

template <typename address_size_t>
std::string masm::ir::dot_equate<address_size_t>::generate_source_string() const
{
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto dot_string = ".EQUATE";
    auto operand_string = argument->string();
	std::string comment = this->comment.value_or("");
    return fmt::format("{:<9}{:<8}{:<12}{}",
		symbol_string,
		dot_string,
		operand_string,
		comment
	);

}

template <typename address_size_t>
void masm::ir::dot_equate<address_size_t>::append_object_code(std::vector<uint8_t>& bytes) const
{
	return;
}

template <typename address_size_t>
bool masm::ir::dot_equate<address_size_t>::tracks_trace_tags() const
{
	return true;
}

/*
 * .WORD
 */
template <typename address_size_t>
masm::ir::dot_word<address_size_t>::dot_word()
{
	this->emits_object_code = true;
}

template <typename address_size_t>
masm::ir::dot_word<address_size_t>::dot_word(const masm::ir::dot_word<address_size_t>& other)
{

}

template <typename address_size_t>
masm::ir::dot_word<address_size_t> &masm::ir::dot_word<address_size_t>::operator=(
	masm::ir::dot_word<address_size_t> other)
{
    swap(*this, other);
    return *this;
}


template <typename address_size_t>
std::shared_ptr<masm::ir::linear_line<address_size_t>> masm::ir::dot_word<address_size_t>::clone() const
{
    return std::make_shared<dot_word<address_size_t> >(*this);
}

template <typename address_size_t>
std::string masm::ir::dot_word<address_size_t>::generate_listing_string() const
{
    // Potentially skip codegen
    std::string code_string = "";
	if(this->emits_object_code) {
		code_string = fmt::format("{:04X}", this->argument->value());
	}


	auto temp = fmt::format("{:<6} {:<6}{}",
		fmt::format("0x{:04X}", this->base_address()),
		code_string,
		generate_source_string()
	);
	
	return temp;
}

template <typename address_size_t>
std::string masm::ir::dot_word<address_size_t>::generate_source_string() const
{
    std::string symbol_string;
    if (this->symbol_entry != nullptr) {
        symbol_string = this->symbol_entry->name + ":";
    }
    auto dot_string = ".BYTE";
    auto operand_string = argument->string();
	std::string comment = this->comment.value_or("");
    return fmt::format("{:<9}{:<8}{:<12}{}",
		symbol_string,
		dot_string,
		operand_string,
		comment
	);

}

template <typename address_size_t>
address_size_t masm::ir::dot_word<address_size_t>::object_code_bytes() const
{
	return 2;
}

template <typename address_size_t>
void masm::ir::dot_word<address_size_t>::append_object_code(std::vector<uint8_t>& bytes) const
{
	bytes.emplace_back((argument->value() >> 8 ) & 0xff);
	bytes.emplace_back(argument->value() & 0xff);
}

template <typename address_size_t>
bool masm::ir::dot_word<address_size_t>::tracks_trace_tags() const
{
	return true;
}