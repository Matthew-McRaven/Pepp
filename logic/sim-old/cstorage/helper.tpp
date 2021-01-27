#include "helper.hpp"

#include <stdexcept>

#include <fmt/core.h>

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
[[noreturn]] void components::storage::oob_read_helper(offset_t offset)
{
	// TODO: Replace libfmt with STL's std::format. 
    throw std::out_of_range(fmt::format("Out of range memory read at: {x}", offset)); 
}

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
[[noreturn]] void components::storage::oob_write_helper(offset_t offset, uint8_t value)
{
	// TODO: Replace libfmt with STL's std::format. 
    throw std::out_of_range(fmt::format("Out of range memory write at: {x}", offset)); 
}
