#pragma once

#include <cstdint>
#include <type_traits>
#include <map>

#include "helper.hpp"
namespace components::storage {
template<typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
class storage_map
{
public:
	storage_map(offset_t max_offset, uint8_t default_value=0);
	void clear(uint8_t fill_val=0);
    // Read / Write functions that may generate signals or trap for IO.
    uint8_t read_byte(offset_t offset) const;
	uint8_t get_byte(offset_t offset) const ;
    void write_byte(offset_t offset, uint8_t value);
    void set_byte(offset_t offset, uint8_t value);

	// Number of bytes contained by this chip
    offset_t max_offset() const noexcept;
    // Change the size of the chip at runtime, to avoid creating and deleting
    // an excessive number of chip instances.
    void resize(offset_t new_offset) noexcept;
private:
	offset_t _max_offset;
	uint8_t _default;
	std::map<offset_t, uint8_t> _storage;
};
}; // End namespace components::storage
#include "map.tpp"