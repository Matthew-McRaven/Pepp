/*
* Memory-mapped input storage device backed by a pubsub Topic.
*/
template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Input<offset_t, val_size_t>::Block(): 
	components::storage::Base<offset_t, val_size_t>(0),
{

}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::Input<offset_t, val_size_t>::clear(val_size_t fill_val)
{
	this-> _last_read_value = fill_val;
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
outcome<val_size_t> components::storage::Input<offset_t, val_size_t>::get(offset_t offset) const
{
	if(offset > this->_max_offset) return oob_read_helper(offset);
	return this->_last_read_value;
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
outcome<void> components::storage::Input<offset_t, val_size_t>::set(offset_t offset, val_size_t value)
{
	if(offset > this->_max_offset) return oob_write_helper(offset, value);
	this->_last_read_value = value;
	return outcome<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
outcome<val_size_t> components::storage::Input<offset_t, val_size_t>::read(offset_t offset) const
{
	if(offset > this->_max_offset) return oob_read_helper(offset);
	auto value = _endpoint.next_value();
	if(!value) return StorageErrc::NoMMInput;
	else return this->_last_read_value = *value;
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
outcome<void> components::storage::Input<offset_t, val_size_t>::write(offset_t offset, val_size_t value)
{
	return StorageErrc::Unwriteable;
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
outcome<void> components::storage::Input<offset_t, val_size_t>::resize(offset_t new_offset)
{
	return StorageErrc::ResizeError;
}
