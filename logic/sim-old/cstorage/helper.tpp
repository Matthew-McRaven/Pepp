#include "helper.hpp"

#include <stdexcept>

#include <fmt/core.h>

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
std::exception_ptr components::storage::oob_read_helper(offset_t offset)
{
	// TODO: Replace libfmt with STL's std::format. 
	auto err_str = fmt::format("Out-of-range-memory read at: {:x}", offset);
    return std::make_exception_ptr(std::out_of_range(err_str)); 
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::exception_ptr components::storage::oob_write_helper(offset_t offset, val_size_t value)
{
	// TODO: Replace libfmt with STL's std::format. 
	auto err_str = fmt::format("Out-of-range-memory write at: {:x}", offset);
    return std::make_exception_ptr(std::out_of_range(err_str)); 
}
