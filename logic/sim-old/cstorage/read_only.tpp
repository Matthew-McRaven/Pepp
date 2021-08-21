#include "storage_error.hpp"
/*
* ReadOnly-based storage device.
*/
template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::ReadOnly<offset_t, enable_history, val_size_t>::ReadOnly(
	std::shared_ptr<components::storage::Base<offset_t, enable_history, val_size_t>> storage,
	components::storage::WriteAttemptPolicy policy):
	components::storage::Base<offset_t, enable_history, val_size_t>(storage->max_offset()),
	_storage(storage), _policy(policy)

{

}


template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::ReadOnly<offset_t, enable_history, val_size_t>::clear(val_size_t fill_val)
{
	_storage->clear();
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<val_size_t> components::storage::ReadOnly<offset_t, enable_history, val_size_t>::get(offset_t offset) const
{
	return _storage->get(offset);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::ReadOnly<offset_t, enable_history, val_size_t>::set(offset_t offset, val_size_t value)
{
	return _storage->set(offset, value);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<val_size_t> components::storage::ReadOnly<offset_t, enable_history, val_size_t>::read(offset_t offset) const
{
	return _storage->read(offset);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::ReadOnly<offset_t, enable_history, val_size_t>::write(offset_t offset, val_size_t value)
{	
	using components::storage::WriteAttemptPolicy;
	switch(_policy) {
	case WriteAttemptPolicy::kIgnore: return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
	case WriteAttemptPolicy::kFail: return status_code(StorageErrc::IllegalWrite);
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<const components::storage::Base<offset_t, enable_history, val_size_t>*>
	components::storage::ReadOnly<offset_t, enable_history, val_size_t>::device_at(offset_t offset) const 
{
	using base_t = components::storage::Base<offset_t, enable_history, val_size_t>;
	// TODO: Change to OOBAccess.
	if(offset > this->_max_offset) return status_code(StorageErrc::OOBRead);
	else return std::const_pointer_cast<const base_t>(_storage)->device_at(offset);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<components::storage::Base<offset_t, enable_history, val_size_t>*>
	components::storage::ReadOnly<offset_t, enable_history, val_size_t>::device_at(offset_t offset) 
{
	// TODO: Change to OOBAccess.
	if(offset > this->_max_offset) return status_code(StorageErrc::OOBRead);
	else return _storage->device_at(offset);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
bool components::storage::ReadOnly<offset_t, enable_history, val_size_t>::deltas_enabled() const
{
	return _storage->deltas_enabled();
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::ReadOnly<offset_t, enable_history, val_size_t>::clear_delta()
{	
	return _storage->clear_delta();
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<std::unique_ptr<components::delta::Base<offset_t, val_size_t>>> components::storage::ReadOnly<offset_t, enable_history, val_size_t>::take_delta()
{	
	return _storage->take_delta();
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::ReadOnly<offset_t, enable_history, val_size_t>::resize(offset_t new_offset)
{

	 auto ret = _storage->resize(new_offset);
	 if(!ret.has_error()) this->_max_offset = new_offset;
	 return ret;
}
