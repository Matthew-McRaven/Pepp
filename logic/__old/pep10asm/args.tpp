#include "args.hpp"

// TODO: Switch to stl format when available.
#include <fmt/core.h>
#include <type_traits>

#include "masm/conversion.hpp"
template <typename address_size_t>
masm::ir::char_argument<address_size_t>::char_argument(std::string char_value) : char_value_(std::move(char_value))
{

}

template <typename address_size_t>
address_size_t masm::ir::char_argument<address_size_t>::argument_value() const
{
	address_size_t val;
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
    return fmt::format("0x{4:X}", hex_value_);
}

template <typename address_size_t>
masm::ir::string_argument<address_size_t>::string_argument(std::string string_value):
	string_value_(std::move(string_value))
{

}

template <typename address_size_t>
address_size_t masm::ir::string_argument<address_size_t>::argument_value() const
{
	address_size_t val;
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
