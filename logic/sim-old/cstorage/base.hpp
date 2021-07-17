#pragma once

#include <cstdint>
#include <vector>
#include <type_traits>

#include "helper.hpp"
#include "outcome_helper.hpp"

namespace components::storage{

template<typename offset_t, bool enable_history=true, typename val_size_t=uint8_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
class Base
{
public:

	// TODO: Rule of 5.
	// TODO: Copy-swap.
    // TODO: Ban copying "C.67: A polymorphic class should suppress copying"
	Base(offset_t max_offset);
	virtual ~Base() noexcept = default;
	virtual void clear(val_size_t fill_val=0) = 0;
    // Read / Write functions that may generate signals or trap for IO.
	virtual outcome<val_size_t> get(offset_t offset) const = 0;
	virtual outcome<void> set(offset_t offset, val_size_t value) = 0;
    virtual outcome<val_size_t> read(offset_t offset) const = 0;
    virtual outcome<void> write(offset_t offset, val_size_t value) = 0;

	// Provide undo functionality if the class has history enabled.
	virtual bool can_undo() const = 0;
	virtual outcome<val_size_t> unread(offset_t offset) = 0;
	virtual outcome<val_size_t> unwrite(offset_t offset) = 0;

	// Number of bytes contained by this chip
    virtual offset_t max_offset() const;
    // Change the size of the chip at runtime, to avoid creating and deleting
    // an excessive number of chip instances.
	// Resizing the underlying storage may indeed throw due to lack of memory or a myriad of other STL problems.
	// If this is the case, there is absolutely nothing we can do to fix application state -- we are out of memory.
	// Just terminate directly.
    virtual outcome<void> resize(offset_t new_offset) = 0;
	
protected:
	offset_t _max_offset;
};

}; // End namespace components::memory

#include "base.tpp"