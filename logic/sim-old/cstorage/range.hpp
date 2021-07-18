#pragma once

#include <cstdint>
#include <vector>
#include <type_traits>

#include "helper.hpp"
#include "components/storage/base.hpp"

namespace components::storage {
// TODO: Allow allocations to "block out" more than 1 byte at a time.
// This would significantly improve performance for large numbers of allocations.
template <typename offset_t, bool enable_history=true, typename val_size_t=uint8_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
class Range: public components::storage::Base<offset_t, enable_history, val_size_t>
{
public:
	// TODO: Rule of 5.
	// TODO: Copy-swap.
	Range(offset_t max_offset, val_size_t default_value=0);
	virtual ~Range() = default;
	void clear(val_size_t fill_val=0) override;
    // Read / Write functions that may generate signals or trap for IO.
	outcome<val_size_t> get(offset_t offset) const override;
	outcome<void> set(offset_t offset, val_size_t value) override;
    outcome<val_size_t> read(offset_t offset) const override;
    outcome<void> write(offset_t offset, val_size_t value) override;

	// Provide  building block of `undo` using layered deltas.
	bool deltas_enabled() const override;
	outcome<void> clear_delta() override;
	outcome<std::unique_ptr<components::delta::Base<offset_t, val_size_t>>> take_delta() override;

    // Change the size of the chip at runtime, to avoid creating and deleting
    // an excessive number of chip instances.
    outcome<void> resize(offset_t new_offset) override;
private:
	val_size_t _default;
	std::vector<components::storage::storage_span<offset_t, val_size_t> > _storage;
	std::unique_ptr<components::delta::Vector<offset_t, val_size_t>> _delta {nullptr};
};
}; // End namespace components::storage
#include "range.tpp"