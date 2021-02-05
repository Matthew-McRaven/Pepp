#include "args.hpp"

// TODO: Switch to stl format when available.
#include <fmt/core.h>
#include <stdexcept>
#include <type_traits>


#include "masm/conversion.hpp"
template <typename address_size_t>
masm::ir::char_argument<address_size_t>::char_argument(std::string char_value) : char_value_(std::move(char_value))
{
    address_size_t dummy;
    if(auto len = masm::byte_string_length(char_value_); len != 1) {
        throw std::logic_error(fmt::format("Character strings may only be 1 byte. Recieved {} bytes.", len));
    }
    else if(!masm::unqouted_string_to_integral(char_value_, 1, dummy)) {
        throw std::logic_error(fmt::format("Character strings must be convertible to uint8_t. Check your escape codes."));     
    }
}

template <typename address_size_t>
address_size_t masm::ir::char_argument<address_size_t>::argument_value() const
{
    // Must initialize to 0, or JS runtime value is undefined.
	address_size_t val = 0;
	masm::unqouted_string_to_integral<address_size_t>(char_value_, 1, val);
	return val;
}

template <typename address_size_t>
std::string masm::ir::char_argument<address_size_t>::argument_string() const
{
    return char_value_;
}

template <typename address_size_t>
masm::ir::dec_argument<address_size_t>::dec_argument(address_size_t dec_value): dec_value_(dec_value)
{

}

template <typename address_size_t>
address_size_t masm::ir::dec_argument<address_size_t>::argument_value() const
{
    return dec_value_;
}

template <typename address_size_t>
std::string masm::ir::dec_argument<address_size_t>::argument_string() const
{
    typename std::make_signed<address_size_t>::type temp = dec_value_;
    return fmt::format("{:d}", temp);
}

template <typename address_size_t>
masm::ir::unsigned_dec_argument<address_size_t>::unsigned_dec_argument(address_size_t dec_value): dec_value_(dec_value)
{

}

template <typename address_size_t>
address_size_t masm::ir::unsigned_dec_argument<address_size_t>::argument_value() const
{
    return dec_value_;
}

template <typename address_size_t>
std::string masm::ir::unsigned_dec_argument<address_size_t>::argument_string() const
{
	typename std::make_unsigned<address_size_t>::type temp = dec_value_;;
    return fmt::format("{:d}", temp);
}

template <typename address_size_t>
masm::ir::hex_argument<address_size_t>::hex_argument(address_size_t hex_value): hex_value_(hex_value)
{

}

template <typename address_size_t>
address_size_t masm::ir::hex_argument<address_size_t>::argument_value() const
{
    return hex_value_;
}

template <typename address_size_t>
std::string masm::ir::hex_argument<address_size_t>::argument_string() const
{
    return fmt::format("0x{:04X}", hex_value_);
}

template <typename address_size_t>
masm::ir::string_argument<address_size_t>::string_argument(std::string string_value):
	string_value_(std::move(string_value))
{
    address_size_t dummy;
    if(auto len = masm::byte_string_length(string_value_); len > 2) {
        throw std::logic_error(fmt::format("Strings may be up to 2 bytes. Recieved {} bytes.", len));
    }
    else if(!masm::unqouted_string_to_integral(string_value_, sizeof(address_size_t), dummy)) {
        throw std::logic_error(fmt::format("Character strings must be convertible to address_size_t. Check your escape codes."));     
    }
}

template <typename address_size_t>
address_size_t masm::ir::string_argument<address_size_t>::argument_value() const
{
    // Must initialize to 0, or JS runtime value is undefined.
	address_size_t val = 0;
	masm::unqouted_string_to_integral<address_size_t>(string_value_, 2, val);
	return val;
}

template <typename address_size_t>
std::string masm::ir::string_argument<address_size_t>::argument_string() const
{
    return string_value_;
}

template <typename address_size_t>
masm::ir::symbol_ref_argument<address_size_t>::symbol_ref_argument(std::shared_ptr<const symbol::SymbolEntry<address_size_t>>  ref_value):
	ref_value_(std::move(ref_value))
{

}

template <typename address_size_t>
address_size_t masm::ir::symbol_ref_argument<address_size_t>::argument_value() const
{
    return ref_value_->getValue();
}

template <typename address_size_t>
std::string masm::ir::symbol_ref_argument<address_size_t>::argument_string() const
{
    return ref_value_->getName();
}

template <typename address_size_t>
std::shared_ptr<const symbol::SymbolEntry<address_size_t> > masm::ir::symbol_ref_argument<address_size_t>::symbol_value()
{
    return ref_value_;
}
