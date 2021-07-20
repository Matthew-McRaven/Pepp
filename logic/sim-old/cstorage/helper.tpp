#include "helper.hpp"

#include <stdexcept>

#include <fmt/core.h>

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
std::exception_ptr components::storage::oob_read_helper(offset_t offset)
{
	// TODO: Replace libfmt with STL's std::format.
	// Number of hex digits per-byte is 2. 
	auto pad = sizeof(offset_t)*2;
	// Since I don't know the pad size at time of writing, must generate a fmt string with correct padding.
	auto err_template = fmt::format("Out-of-range-memory read at offset: 0x{{:0{}x}}", pad);
	// Fill in the address of the error template.
	auto err_str = fmt::format(err_template, offset);
    return std::make_exception_ptr(std::out_of_range(err_str)); 
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::exception_ptr components::storage::oob_write_helper(offset_t offset, val_size_t value)
{
	// TODO: Replace libfmt with STL's std::format.
	// Number of hex digits per-byte is 2. 
	auto pad = sizeof(offset_t)*2;
	// Since I don't know the pad size at time of writing, must generate a fmt string with correct padding.
	auto err_template = fmt::format("Out-of-range-memory read at offset: 0x{{:0{}x}}", pad);
	// Fill in the address of the error template.
	auto err_str = fmt::format(err_template, offset);
    return std::make_exception_ptr(std::out_of_range(err_str)); 
}
