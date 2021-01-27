#pragma once

#include <cstdint>
#include <vector>
#include <type_traits>

#include "helper.hpp"
#include "components/storage/base.hpp"

namespace components::storage {
// TODO: Allow allocations to "block out" more than 1 byte at a time.
// This would significantly improve performance for large numbers of allocations.
template <typename offset_t, typename val_size_t=uint8_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
class storage_range: public components::storage::storage_base<offset_t, val_size_t>
{
public:
	// TODO: Rule of 5.
	// TODO: Copy-swap.
	storage_range(offset_t max_offset, val_size_t default_value=0);
	virtual ~storage_range() = default;
	void clear(val_size_t fill_val=0) override;
    // Read / Write functions that may generate signals or trap for IO.
    val_size_t read(offset_t offset) const override;
	val_size_t get(offset_t offset) const override;
    void write(offset_t offset, val_size_t value) override;
    void set(offset_t offset, val_size_t value) override;

    // Change the size of the chip at runtime, to avoid creating and deleting
    // an excessive number of chip instances.
    void resize(offset_t new_offset) noexcept override;
private:
	val_size_t _default;
	std::vector<components::storage::storage_span<offset_t, val_size_t> > _storage;
};
}; // End namespace components::storage
#include "range.tpp"