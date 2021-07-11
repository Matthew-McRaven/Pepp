#pragma once

#include <cstdint>
#include <vector>
#include <type_traits>

#include "helper.hpp"

namespace components::storage{

// TODO: Refactor using a memory span.
template<typename offset_t, typename val_size_t=uint8_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
class Base
{
public:
	// TODO: Rule of 5.
	// TODO: Copy-swap.
    // TODO: Ban copying "C.67: A polymorphic class should suppress copying"
	Base(offset_t max_offset);
	virtual ~Base() = default;
	virtual void clear(val_size_t fill_val=0) = 0;
    // Read / Write functions that may generate signals or trap for IO.
    virtual val_size_t read(offset_t offset) const = 0;
	virtual val_size_t get(offset_t offset) const = 0;
    virtual void write(offset_t offset, val_size_t value) = 0;
    virtual void set(offset_t offset, val_size_t value) = 0;

	// Number of bytes contained by this chip
    virtual offset_t max_offset() const noexcept;
    // Change the size of the chip at runtime, to avoid creating and deleting
    // an excessive number of chip instances.
    virtual void resize(offset_t new_offset) noexcept = 0;
	
protected:
	offset_t _max_offset;
};

}; // End namespace components::memory

#include "base.tpp"