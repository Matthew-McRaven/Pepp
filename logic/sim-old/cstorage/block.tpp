/*
* Block-based storage device.
*/
template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Block<offset_t, val_size_t>::Block(offset_t max_offset): 
	components::storage::Base<offset_t, val_size_t>(max_offset),
	_storage(std::vector<val_size_t>(this->_max_offset + 1))
{

}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::Block<offset_t, val_size_t>::clear(val_size_t fill_val)
{
	std::fill(_storage.begin(), _storage.end(), fill_val);
}
template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
val_size_t components::storage::Block<offset_t, val_size_t>::read(offset_t offset) const
{
	if(offset > this->_max_offset) oob_read_helper(offset);
	else return _storage.at(offset);
	return 0; // Never reached, since oob throws. Needed to silence compiler warning.
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
val_size_t components::storage::Block<offset_t, val_size_t>::get(offset_t offset) const
{
	if(offset > this->_max_offset) oob_read_helper(offset);
	else return _storage.at(offset);
	return 0; // Never reached, since oob throws. Needed to silence compiler warning.
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::Block<offset_t, val_size_t>::write(offset_t offset, val_size_t value)
{
	if(offset > this->_max_offset) oob_write_helper(offset, value);
	else _storage[offset] = value;
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::Block<offset_t, val_size_t>::set(offset_t offset, val_size_t value)
{
	if(offset > this->_max_offset) oob_write_helper(offset, value);
	else _storage[offset] = value;
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::Block<offset_t, val_size_t>::resize(offset_t new_offset) noexcept
{
	this->_max_offset = new_offset;
	_storage.resize(new_offset+1);
	clear();
}
