/*
 * Map-based storage device.
 */
template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Map<offset_t, val_size_t>::Map(offset_t max_offset, val_size_t default_value): 
	components::storage::Base<offset_t, val_size_t>(max_offset),
	_default(default_value), _storage()
{

}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::Map<offset_t, val_size_t>::clear(val_size_t fill_val)
{
	_storage.clear();
	_default = fill_val;
}
template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
val_size_t components::storage::Map<offset_t, val_size_t>::read(offset_t offset) const
{
	if(offset > this->_max_offset) oob_read_helper(offset);
	else if(auto key = _storage.find(offset); key != _storage.end()) return key->second;
	else return _default;
	return 0; // Never reached, since oob throws. Needed to silence compiler warning.
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
val_size_t components::storage::Map<offset_t, val_size_t>::get(offset_t offset) const
{
	if(offset > this->_max_offset) oob_read_helper(offset);
	else if(auto key = _storage.find(offset); key != _storage.end()) return key->second;
	else return _default;
	return 0; // Never reached, since oob throws. Needed to silence compiler warning.
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::Map<offset_t, val_size_t>::write(offset_t offset, val_size_t value)
{
	if(offset > this->_max_offset) oob_write_helper(offset, value);
	else _storage[offset] = value;
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::Map<offset_t, val_size_t>::set(offset_t offset, val_size_t value)
{
	if(offset > this->_max_offset) oob_write_helper(offset, value);
	else _storage[offset] = value;
}


template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::Map<offset_t, val_size_t>::resize(offset_t new_offset) noexcept
{
	this->_max_offset = new_offset;
	clear();
}