#pragma once

#include <cstdint>
#include <vector>
#include <type_traits>

#include "base.hpp"
#include "helper.hpp"

namespace components::storage{

// How should a read-only adaptor behave when a write is attempted?
enum class WriteAttemptPolicy
{
	kIgnore, // Do not change the underlying storage, and return OUTCOME_V2_NAMESPACE::in_place_type<void>.
	kFail, // Do not change the underlying storage, and return a StorageErrc::IllegalWrite.
};

template<typename offset_t, bool enable_history=true, typename val_size_t=uint8_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
class ReadOnly: public components::storage::Base<offset_t, enable_history, val_size_t>
{
public:
	// TODO: Rule of 5.
	// TODO: Copy-swap.
	// Allocating underlying storage can fail, but we can't recover from this error. Terminate directly.
	ReadOnly(std::shared_ptr<components::storage::Base<offset_t, enable_history, val_size_t>> storage,
		WriteAttemptPolicy policy);
    virtual ~ReadOnly() noexcept = default;
	void clear(val_size_t fill_val=0) override;
    // Read / Write functions that may generate signals or trap for IO.
	result<val_size_t> get(offset_t offset) const override;
	result<void> set(offset_t offset, val_size_t value) override;
    result<val_size_t> read(offset_t offset) const override;
    result<void> write(offset_t offset, val_size_t value) override;

	// Return underlying storage device. This allows MMIO devices to still be accessed even when marked as RO.
	// If offset is out of bounds, return an OOB-related error code.
	virtual result<const Base<offset_t, enable_history, val_size_t>*> device_at(offset_t offset) const override;
	virtual result<Base<offset_t, enable_history, val_size_t>*> device_at(offset_t offset) override;

	// Provide  building block of `undo` using layered deltas.
	bool deltas_enabled() const override;
	result<void> clear_delta() override;
	result<std::unique_ptr<components::delta::Base<offset_t, val_size_t>>> take_delta() override;

	// Number of bytes contained by this chip
    // offset_t max_offset() const noexcept;
    // Change the size of the chip at runtime, to avoid creating and deleting
    // an excessive number of chip instances.
    result<void> resize(offset_t new_offset) override;
private:
	std::shared_ptr<components::storage::Base<offset_t, enable_history, val_size_t>> _storage;
	WriteAttemptPolicy _policy;
};

}; // End namespace components::memory

#include "read_only.tpp"