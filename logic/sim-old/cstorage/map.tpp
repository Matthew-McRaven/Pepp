/*
 * Map-based storage device.
 */
template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
components::storage::storage_map<offset_t>::storage_map(offset_t max_offset, uint8_t default_value): 
	_max_offset(max_offset), _default(default_value), _storage()
{

}

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
void components::storage::storage_map<offset_t>::clear(uint8_t fill_val)
{
	_storage.clear();
	_default = fill_val;
}
template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
uint8_t components::storage::storage_map<offset_t>::read_byte(offset_t offset) const
{
	if(offset > _max_offset) oob_read_helper(offset);
	else if(auto key = _storage.find(offset); key != _storage.end()) return key->second;
	else return _default;
}

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
uint8_t components::storage::storage_map<offset_t>::get_byte(offset_t offset) const
{
	if(offset > _max_offset) oob_read_helper(offset);
	else if(auto key = _storage.find(offset); key != _storage.end()) return key->second;
	else return _default;
}

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
void components::storage::storage_map<offset_t>::write_byte(offset_t offset, uint8_t value)
{
	if(offset > _max_offset) oob_write_helper(offset, value);
	else _storage[offset] = value;
}

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
void components::storage::storage_map<offset_t>::set_byte(offset_t offset, uint8_t value)
{
	if(offset > _max_offset) oob_write_helper(offset, value);
	else _storage[offset] = value;
}


template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
offset_t components::storage::storage_map<offset_t>::max_offset() const noexcept
{
	return _max_offset;
}

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
void components::storage::storage_map<offset_t>::resize(offset_t new_offset) noexcept
{
	_max_offset = new_offset;
	clear();
}