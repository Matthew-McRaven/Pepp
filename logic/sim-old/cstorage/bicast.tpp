template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::storage_bicast<offset_t, val_size_t>::storage_bicast(storager_ptr_t primary, storager_ptr_t replica): 
	components::storage::storage_base<offset_t, val_size_t>(primary->max_offset()),
	_primary(primary), _replica(replica)
{
	assert(primary->max_offset() == replica->max_offset());
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::storage_bicast<offset_t, val_size_t>::clear(val_size_t fill_val)
{
	_primary->clear(fill_val);
	_replica->clear(fill_val);
}
template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
val_size_t components::storage::storage_bicast<offset_t, val_size_t>::read(offset_t offset) const
{
	val_size_t primary_value = primary->read(offset);
	val_size_t replica_value = replica->read(offset);
	return primary_value;
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
val_size_t components::storage::storage_bicast<offset_t, val_size_t>::get(offset_t offset) const
{
	val_size_t primary_value = primary->get(offset);
	val_size_t replica_value = replica->get(offset);
	return primary_value;	
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::storage_bicast<offset_t, val_size_t>::write(offset_t offset, val_size_t value)
{
	primary->write(offset, value);
	replica->write(offset, value);
	return primary_value;
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::storage_bicast<offset_t, val_size_t>::set(offset_t offset, val_size_t value)
{
	primary->set(offset, value);
	replica->set(offset, value);
	return primary_value;;
}

template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::storage_bicast<offset_t, val_size_t>::resize(offset_t new_offset) noexcept
{
	primary->clear();
	replica->clear*()
}
