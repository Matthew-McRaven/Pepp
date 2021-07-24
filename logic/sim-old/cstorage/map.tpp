/*
 * Map-based storage device.
 */
template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Map<offset_t, enable_history, val_size_t>::Map(offset_t max_offset, val_size_t default_value)
	requires(enable_history): components::storage::Base<offset_t, enable_history, val_size_t>(max_offset),
	_default(default_value), _storage(),
	_delta(std::make_unique<components::delta::Vector<offset_t, val_size_t>>(*this))
{

}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Map<offset_t, enable_history, val_size_t>::Map(offset_t max_offset, val_size_t default_value)
	requires(!enable_history): components::storage::Base<offset_t, enable_history, val_size_t>(max_offset),
	_default(default_value), _storage(), _delta(nullptr)
{

}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::Map<offset_t, enable_history, val_size_t>::clear(val_size_t fill_val)
{
	_storage.clear();
	_default = fill_val;
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<val_size_t> components::storage::Map<offset_t, enable_history, val_size_t>::get(offset_t offset) const
{
	if(offset > this->_max_offset) return oob_read_helper(offset);
	else if(auto key = _storage.find(offset); key != _storage.end()) return key->second;
	else return _default;
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::Map<offset_t, enable_history, val_size_t>::set(offset_t offset, val_size_t value)
{
	if(offset > this->_max_offset) return oob_write_helper(offset, value);
	else _storage[offset] = value;
	return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<val_size_t> components::storage::Map<offset_t, enable_history, val_size_t>::read(offset_t offset) const
{
	// No need to perform any delta computation, as reading never changes the state of non-memory-mapped storage.
	return get(offset);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::Map<offset_t, enable_history, val_size_t>::write(offset_t offset, val_size_t value)
{
	if constexpr(enable_history) {
		// This is a redundant check with set(), but it is very important that we don't generate illegal deltas.
		if(offset > this->_max_offset) return oob_write_helper(offset, value);
		_delta->add_delta(offset, get(offset).value(), value);
	}
	return set(offset, value);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
bool components::storage::Map<offset_t, enable_history, val_size_t>::deltas_enabled() const
{
	return enable_history;
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::Map<offset_t, enable_history, val_size_t>::clear_delta()
{	
	if constexpr(enable_history) {
		_delta->clear();
		return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
	}
	else {
		return status_code(StorageErrc::DeltaDisabled);
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<std::unique_ptr<components::delta::Base<offset_t, val_size_t>>> components::storage::Map<offset_t, enable_history, val_size_t>::take_delta()
{	
	if constexpr(enable_history) {
		// Helper for enabling std::swap.
		using std::swap;
		auto ret = std::make_unique<components::delta::Vector<offset_t, val_size_t>>(*this);
		swap(ret, _delta);
		return {std::move(ret)};
	}
	else {
		return status_code(StorageErrc::DeltaDisabled);
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::Map<offset_t, enable_history, val_size_t>::resize(offset_t new_offset)
{
	this->_max_offset = new_offset;
	clear();
	return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
}