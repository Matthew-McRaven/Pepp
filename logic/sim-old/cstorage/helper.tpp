#include "helper.hpp"

#include <stdexcept>

#include <fmt/core.h>

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
void components::storage::oob_read_helper(offset_t offset)
{
	// TODO: Replace libfmt with STL's std::format. 
    throw std::out_of_range(fmt::format("Out of range memory read at: {x}", offset)); 
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::oob_write_helper(offset_t offset, val_size_t value)
{
	// TODO: Replace libfmt with STL's std::format. 
    throw std::out_of_range(fmt::format("Out of range memory write at: {x}", offset)); 
}
