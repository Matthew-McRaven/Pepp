#pragma once

#include <cstdint>
#include <vector>
#include <type_traits>

#include "base.hpp"
#include "helper.hpp"

namespace components::storage{

// TODO: Refactor using a memory span.
template<typename offset_t, typename val_size_t=uint8_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
class Block: public components::storage::Base<offset_t, val_size_t>
{
public:
	// TODO: Rule of 5.
	// TODO: Copy-swap.
	// Allocating underlying storage can fail, but we can't recover from this error. Terminate directly.
	Block(offset_t max_offset);
    virtual ~Block() noexcept = default;
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
    outcome<void> resize(offset_t new_offset) override;
private:
	std::vector<val_size_t> _storage;
};

}; // End namespace components::memory

#include "block.tpp"