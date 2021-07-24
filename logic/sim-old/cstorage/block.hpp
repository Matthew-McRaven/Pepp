#pragma once

#include <cstdint>
#include <vector>
#include <type_traits>

#include "components/delta/vector.hpp"
#include "base.hpp"
#include "helper.hpp"

namespace components::storage{

// TODO: Refactor using a memory span.
template<typename offset_t, bool enable_history=true, typename val_size_t=uint8_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
class Block: public components::storage::Base<offset_t, enable_history, val_size_t>
{
public:
	// TODO: Rule of 5.
	// TODO: Copy-swap.
	// Allocating underlying storage can fail, but we can't recover from this error. Terminate directly.
	Block(offset_t max_offset) requires(enable_history);
	Block(offset_t max_offset) requires(!enable_history);
    virtual ~Block() noexcept = default;
	void clear(val_size_t fill_val=0) override;
    // Read / Write functions that may generate signals or trap for IO.
	result<val_size_t> get(offset_t offset) const override;
	result<void> set(offset_t offset, val_size_t value) override;
    result<val_size_t> read(offset_t offset) const override;
    result<void> write(offset_t offset, val_size_t value) override;

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
	std::vector<val_size_t> _storage;
	std::unique_ptr<components::delta::Vector<offset_t, val_size_t>> _delta {nullptr};
};

}; // End namespace components::memory

#include "block.tpp"