#pragma once

#include "components/delta/mmio.hpp"
#include "base.hpp"
#include "helper.hpp"
#include "pubsub.hpp"

namespace components::storage{

template<typename offset_t, bool enable_history, typename val_size_t=uint8_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
class Input: public components::storage::Base<offset_t, enable_history, val_size_t>
{
public:
	// TODO: Rule of 5.
	// TODO: Copy-swap.
	// MMIO devices always have a max size of 1 offset.
	Input(val_size_t default_value) requires(enable_history);
	Input(val_size_t default_value) requires(!enable_history);
    virtual ~Input() = default;
	void clear(val_size_t fill_val=0) override;
	
	// Read / Write functions that may generate signals or trap for IO.
	outcome<val_size_t> get(offset_t offset) const override;
	outcome<void> set(offset_t offset, val_size_t value) override;
    outcome<val_size_t> read(offset_t offset) const override;
	outcome<void> write(offset_t offset, val_size_t value) override;
	// Needed to add input to device.
	std::shared_ptr<typename components::storage::Channel<offset_t, val_size_t>::Endpoint> endpoint();

	// Provide  building block of `undo` using layered deltas.
	bool deltas_enabled() const override;
	outcome<void> clear_delta() override;
	outcome<std::unique_ptr<components::delta::Base<offset_t, val_size_t>>> take_delta() override;

	// Number of bytes contained by this chip
    // offset_t max_offset() const noexcept;
    // Change the size of the chip at runtime, to avoid creating and deleting
    // an excessive number of chip instances.
    outcome<void> resize(offset_t new_offset) override;
private:
	// Must be shared_ptr, since Channel uses enable_shared_from_this.
	std::shared_ptr<components::storage::Channel<offset_t, val_size_t>> _storage;

	// Must be mutable for read caching to work.
	mutable val_size_t _last_read_value;
	
	std::shared_ptr<typename components::storage::Channel<offset_t, val_size_t>::Endpoint> _endpoint;

	// Allow delta object to modify out endpoint directly.
	friend class components::delta::Input<offset_t, val_size_t>;
	std::unique_ptr<components::delta::Input<offset_t, val_size_t>> _delta {nullptr};
};

}; // End namespace components::storage

#include "mmio.tpp"