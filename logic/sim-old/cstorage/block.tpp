/*
* Block-based storage device.
*/
template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
components::storage::storage_block<offset_t>::storage_block(offset_t max_offset): _max_offset(max_offset),
	_storage(std::vector<uint8_t>(_max_offset + 1))
{

}

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
void components::storage::storage_block<offset_t>::clear(uint8_t fill_val)
{
	std::fill(_storage.begin(), _storage.end(), fill_val);
}
template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
uint8_t components::storage::storage_block<offset_t>::read_byte(offset_t offset) const
{
	if(offset > _max_offset) oob_read_helper(offset);
	else return _storage.at(offset);
}

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
uint8_t components::storage::storage_block<offset_t>::get_byte(offset_t offset) const
{
	if(offset > _max_offset) oob_read_helper(offset);
	else return _storage.at(offset);
}

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
void components::storage::storage_block<offset_t>::write_byte(offset_t offset, uint8_t value)
{
	if(offset > _max_offset) oob_write_helper(offset, value);
	else _storage[offset] = value;
}

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
void components::storage::storage_block<offset_t>::set_byte(offset_t offset, uint8_t value)
{
	if(offset > _max_offset) oob_write_helper(offset, value);
	else _storage[offset] = value;
}


template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
offset_t components::storage::storage_block<offset_t>::max_offset() const noexcept
{
	return _max_offset;
}

template <typename offset_t>
	requires (components::storage::UnsignedIntegral<offset_t>)
void components::storage::storage_block<offset_t>::resize(offset_t new_offset) noexcept
{
	_max_offset = new_offset;
	_storage.resize(new_offset+1);
	clear();
}
