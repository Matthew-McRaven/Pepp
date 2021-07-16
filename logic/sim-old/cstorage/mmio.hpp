#pragma once

#include "base.hpp"
#include "helper.hpp"
#include "pubsub.hpp"

namespace components::storage 
{
namespace components::storage{

template<typename offset_t, typename val_size_t=uint8_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
class Input: public components::storage::Base<offset_t, val_size_t>
{
public:
	// TODO: Rule of 5.
	// TODO: Copy-swap.
	// MMIO devices always have a max size of 1 offset.
	Input();
    virtual ~Input() = default;
	void clear(val_size_t fill_val=0) override;
	
	// Read / Write functions that may generate signals or trap for IO.
	outcome<val_size_t> get(offset_t offset) const override;
	outcome<void> set(offset_t offset, val_size_t value) override;
    outcome<val_size_t> read(offset_t offset) const override;
	outcome<void> write(offset_t offset, val_size_t value) override;

	// Number of bytes contained by this chip
    // offset_t max_offset() const noexcept;
    // Change the size of the chip at runtime, to avoid creating and deleting
    // an excessive number of chip instances.
    outcome<void> resize(offset_t new_offset) noexcept override;
private:
	components::storage::Channel<offset_t, val_size_t> _storage;

	val_size_t _last_read_value;
	components::storage::Channel<offset_t, val_size_t>::Endpoint _endpoint;
};

}; // End namespace components::storage

#include "mmio.tpp"