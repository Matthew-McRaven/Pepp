#include "args.hpp"

// TODO: Switch to stl format when available.
#include <fmt/core.h>
#include <stdexcept>
#include <type_traits>


#include "masm/conversion.hpp"

/*
 * Character Argument
 */ 
template <typename address_size_t>
masm::ir::char_argument<address_size_t>::char_argument(std::string char_value) : value_(std::move(char_value))
{
    address_size_t dummy;
    if(auto len = masm::byte_string_length(value_); len != 1) {
        throw std::logic_error(fmt::format("Character strings may only be 1 byte. Recieved {} bytes.", len));
    }
    else if(!masm::unqouted_string_to_integral(value_, 1, dummy)) {
        throw std::logic_error(fmt::format("Character strings must be convertible to uint8_t. Check your escape codes."));     
    }
}

template <typename address_size_t>
address_size_t masm::ir::char_argument<address_size_t>::value() const
{
    // Must initialize to 0, or JS runtime value is undefined.
	address_size_t val = 0;
	masm::unqouted_string_to_integral<address_size_t>(value_, 1, val);
	return val;
}

template <typename address_size_t>
std::string masm::ir::char_argument<address_size_t>::string() const
{
    return value_;
}

template <typename address_size_t>
bool masm::ir::char_argument<address_size_t>::fits_in(std::size_t num_bytes) const 
{
    return num_bytes > 0;
};

/*
 * Signed Decimal Argument
 */ 
template <typename address_size_t>
masm::ir::dec_argument<address_size_t>::dec_argument(address_size_t dec_value): value_(dec_value)
{

}

template <typename address_size_t>
address_size_t masm::ir::dec_argument<address_size_t>::value() const
{
    return value_;
}

template <typename address_size_t>
std::string masm::ir::dec_argument<address_size_t>::string() const
{
    typename std::make_signed<address_size_t>::type temp = value_;
    return fmt::format("{:d}", temp);
}

template <typename address_size_t>
bool masm::ir::dec_argument<address_size_t>::fits_in(std::size_t num_bytes) const 
{
    if(num_bytes >= sizeof(address_size_t)) return true;
    else if(num_bytes == 0) return false;

    // Let's type pun to a signed number of the same size (probably UB).
    using signed_address_size_t = typename std::make_signed<address_size_t>::type;
    signed_address_size_t signed_value = static_cast<signed_address_size_t>(value_);
    // Negative, so compare against -2**(n-1)-1
    if(signed_value < 0) return -1*(1 << (8*num_bytes - 1)) -1 <= signed_value;
    // Positive, so compare against 2**(n-1)
    else return value_ < (1 << (8*num_bytes - 1));
};

/*
 * Unsigned Decimal Argument
 */ 
template <typename address_size_t>
masm::ir::unsigned_dec_argument<address_size_t>::unsigned_dec_argument(address_size_t dec_value): value_(dec_value)
{

}

template <typename address_size_t>
address_size_t masm::ir::unsigned_dec_argument<address_size_t>::value() const
{
    return value_;
}

template <typename address_size_t>
std::string masm::ir::unsigned_dec_argument<address_size_t>::string() const
{
	typename std::make_unsigned<address_size_t>::type temp = value_;
    return fmt::format("{:d}", temp);
}

template <typename address_size_t>
bool masm::ir::unsigned_dec_argument<address_size_t>::fits_in(std::size_t num_bytes) const 
{
    if(num_bytes >= sizeof(address_size_t)) return true;
    else if(num_bytes == 0) return false;
    else return value_ < (1 << 8*num_bytes); 
};

/*
 *  Hexadecimal Argument
 */ 
template <typename address_size_t>
masm::ir::hex_argument<address_size_t>::hex_argument(address_size_t hex_value): value_(hex_value)
{

}

template <typename address_size_t>
address_size_t masm::ir::hex_argument<address_size_t>::value() const
{
    return value_;
}

template <typename address_size_t>
std::string masm::ir::hex_argument<address_size_t>::string() const
{
    return fmt::format("0x{:04X}", value_);
}

template <typename address_size_t>
bool masm::ir::hex_argument<address_size_t>::fits_in(std::size_t num_bytes) const 
{
    if(num_bytes >= sizeof(address_size_t)) return true;
    else if(num_bytes == 0) return false;
    else return value_ < (1 << (num_bytes * 8)); 
};

/*
 * Small String Argument
 */ 

template <typename address_size_t>
masm::ir::string_argument<address_size_t>::string_argument(std::string string_value):
	value_(std::move(string_value))
{

    address_size_t dummy;
    if(auto len = masm::byte_string_length(value_); len > sizeof(address_size_t)) {
        throw std::logic_error(fmt::format("Strings may be up to {} bytes. Recieved {} bytes.", sizeof(address_size_t), len));
    }
    else if(!masm::unqouted_string_to_integral(value_, sizeof(address_size_t), dummy)) {
        throw std::logic_error(fmt::format("Character strings must be convertible to address_size_t. Check your escape codes."));     
    }
}

template <typename address_size_t>
address_size_t masm::ir::string_argument<address_size_t>::value() const
{
    // Must initialize to 0, or JS runtime value is undefined.
	address_size_t val = 0;
	masm::unqouted_string_to_integral<address_size_t>(value_, 2, val);
	return val;
}

template <typename address_size_t>
std::string masm::ir::string_argument<address_size_t>::string() const
{
    return value_;
}

template <typename address_size_t>
bool masm::ir::string_argument<address_size_t>::fits_in(std::size_t num_bytes) const {
    return masm::byte_string_length(value_) <= num_bytes;
};

/*
 * Unlimited String Argument
 */ 
template <typename address_size_t>
masm::ir::ascii_argument<address_size_t>::ascii_argument(std::string string_value, size_t max_size):
	value_(std::move(string_value))
{

    address_size_t dummy;
    if(auto len = masm::byte_string_length(value_); len > max_size) {
        throw std::logic_error(fmt::format("Strings may be up to {} bytes. Recieved {} bytes.", max_size, len));
    }
}

template <typename address_size_t>
address_size_t masm::ir::ascii_argument<address_size_t>::value() const
{
    throw std::logic_error("Unsupported operation!");
}

template <typename address_size_t>
std::string masm::ir::ascii_argument<address_size_t>::string() const
{
    return value_;
}

template <typename address_size_t>
std::vector<uint8_t> masm::ir::ascii_argument<address_size_t>::bytes() const
{
    return masm::byte_vector(value_);
}

template <typename address_size_t>
bool masm::ir::ascii_argument<address_size_t>::fits_in(std::size_t num_bytes) const 
{
    if(num_bytes == 0 ) return false;
    return masm::byte_string_length(value_) <= num_bytes;
};

/*
 * Symbol reference argument
 */ 
template <typename address_size_t>
masm::ir::symbol_ref_argument<address_size_t>::symbol_ref_argument(std::shared_ptr<const symbol::SymbolEntry<address_size_t>>  ref_value):
	value_(std::move(ref_value))
{

}

template <typename address_size_t>
address_size_t masm::ir::symbol_ref_argument<address_size_t>::value() const
{
    return value_->getValue();
}

template <typename address_size_t>
std::string masm::ir::symbol_ref_argument<address_size_t>::string() const
{
    return value_->getName();
}

template <typename address_size_t>
std::shared_ptr<const symbol::SymbolEntry<address_size_t> > masm::ir::symbol_ref_argument<address_size_t>::symbol_value()
{
    return value_;
}

template <typename address_size_t>
bool masm::ir::symbol_ref_argument<address_size_t>::fits_in(std::size_t num_bytes) const {
    if(num_bytes == 0) return false;
    return sizeof(address_size_t) <= num_bytes;
};