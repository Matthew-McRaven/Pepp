#include "helper.hpp"

#include <stdexcept>

#include <fmt/core.h>

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
StorageErrc components::storage::oob_read_helper(offset_t offset)
{
	return StorageErrc::OOBRead;
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
StorageErrc components::storage::oob_write_helper(offset_t offset, val_size_t value)
{
	return StorageErrc::OOBWrite;
}
