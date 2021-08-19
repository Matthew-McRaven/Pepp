#include "storage_error.hpp"
#include "components/delta/grouped.hpp"
/*
* Layered-based storage device.
*/
template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Layered<offset_t, enable_history, val_size_t>::Layered(offset_t max_offset,
	val_size_t default_value, ReadMiss read_policy, WriteMiss write_policy) requires(enable_history): 
	components::storage::Base<offset_t, enable_history, val_size_t>(max_offset),
	_storage({}), _default_value(default_value), _read_policy(read_policy), _write_policy(write_policy)
{

}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Layered<offset_t, enable_history, val_size_t>::Layered(offset_t max_offset,
	val_size_t default_value, ReadMiss read_policy, WriteMiss write_policy) requires(!enable_history): 
	components::storage::Base<offset_t, enable_history, val_size_t>(max_offset),
	_storage({}), _default_value(default_value), _read_policy(read_policy), _write_policy(write_policy)
{

}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::Layered<offset_t, enable_history, val_size_t>::append_storage(
	offset_t offset, storage_t storage)
{	
	if(offset+storage->max_offset() > (2<< (8*sizeof(offset_t)))) return status_code(StorageErrc::IllegalInsert);
	auto tuple = std::tuple<offset_t, storage_t>{offset, storage};
	_storage.emplace_back(tuple);
	return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<offset_t> components::storage::Layered<offset_t, enable_history, val_size_t>::storage_to_offset(
	const components::storage::Base<offset_t, enable_history, val_size_t>* to_find) const
{	
	for(auto& [offset, storage] : _storage)
		if(storage.get() == to_find) return offset;
	return status_code(StorageErrc::NoSuchDevice);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
auto components::storage::Layered<offset_t, enable_history, val_size_t>::contained_storage() const 
	-> decltype(components::storage::Layered<offset_t, enable_history, val_size_t>::storage_range_t{})
{	
	return storage_range_t(_storage.cbegin(), _storage.cend());
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::Layered<offset_t, enable_history, val_size_t>::clear(val_size_t fill_val)
{
	for(auto& [offset, storage] : _storage) storage->clear(fill_val);
	
	if constexpr(enable_history) {
		auto ret = clear_delta();
		// If there's an error because there's no deltas enabled, we don't care.
		if(ret.has_error() && ret.error() != status_code(StorageErrc::DeltaDisabled)) {
			// Unrecoverable delta issue, kill application.
			ret.value();
		}
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<val_size_t> components::storage::Layered<offset_t, enable_history, val_size_t>::get(offset_t offset) const
{
	
	if(offset > this->_max_offset) return oob_read_helper(offset);
	else {
		for(auto& [_storage_offset, storage] : _storage) {
			auto adjusted_offset = offset - _storage_offset;
			if(_storage_offset <= offset && adjusted_offset <= storage->max_offset()) 
				return storage->get(adjusted_offset);
		}
	}
	switch(this->_read_policy)
	{
	case ReadMiss::kOOB: return oob_read_helper(offset);
	case ReadMiss::kDefaultValue: return _default_value;
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::Layered<offset_t, enable_history, val_size_t>::set(offset_t offset, val_size_t value)
{
	if(offset > this->_max_offset) return oob_write_helper(offset, value);
	else {
		for(auto& [_storage_offset, storage] : _storage) {
			auto adjusted_offset = offset - _storage_offset;
			if(_storage_offset <= offset && adjusted_offset <= storage->max_offset()) 
				return storage->set(adjusted_offset, value);
		}
	}
	switch(this->_write_policy)
	{
	case WriteMiss::kOOB: return oob_write_helper(offset, value);
	case WriteMiss::kIgnore: return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<val_size_t> components::storage::Layered<offset_t, enable_history, val_size_t>::read(offset_t offset) const
{
	if(offset > this->_max_offset) return oob_read_helper(offset);
	else {
		for(auto& [_storage_offset, storage] : _storage) {
			auto adjusted_offset = offset - _storage_offset;
			if(_storage_offset <= offset && adjusted_offset <= storage->max_offset()) 
				return storage->read(adjusted_offset);
		}
	}
	switch(this->_read_policy)
	{
	case ReadMiss::kOOB: return oob_read_helper(offset);
	case ReadMiss::kDefaultValue: return _default_value;
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::Layered<offset_t, enable_history, val_size_t>::write(offset_t offset, val_size_t value)
{	
	if(offset > this->_max_offset) return oob_write_helper(offset, value);
	else {
		for(auto& [_storage_offset, storage] : _storage) {
			auto adjusted_offset = offset - _storage_offset;
			if(_storage_offset <= offset && adjusted_offset <= storage->max_offset()) 
				return storage->write(adjusted_offset, value);
		}
	}
	switch(this->_write_policy)
	{
	case WriteMiss::kOOB: return oob_write_helper(offset, value);
	case WriteMiss::kIgnore: return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
bool components::storage::Layered<offset_t, enable_history, val_size_t>::deltas_enabled() const
{
	return enable_history;
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::Layered<offset_t, enable_history, val_size_t>::clear_delta()
{	
	if constexpr(enable_history) {
		for(auto& [_, storage] : _storage) {
			// We can't handle a cascading failure to clear deltas, so let the failing delta throw.
			storage->clear_delta().value();
		}
		return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
	}
	else {
		return status_code(StorageErrc::DeltaDisabled);
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<std::unique_ptr<components::delta::Base<offset_t, val_size_t>>> components::storage::Layered<offset_t, enable_history, val_size_t>::take_delta()
{	
	if constexpr(enable_history) {
		auto ret = std::make_unique<components::delta::Grouped<offset_t, val_size_t>>(this);
		// We can't handle a failing delta, so just throw is there is a failure.
		for(auto &[_, storage] : _storage) ret->add_delta(std::move(storage->take_delta().value()));
		return {std::move(ret)};
	}
	else {
		return status_code(StorageErrc::DeltaDisabled);
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::Layered<offset_t, enable_history, val_size_t>::resize(offset_t new_offset)
{
	size_t max_offset = 0;
	for(auto& [offset, storage] : _storage) max_offset = std::max(max_offset, std::size_t(offset) + storage->max_offset());
	if(max_offset > new_offset) return status_code(StorageErrc::ResizeTooSmall);
	this->_max_offset = new_offset;

	clear();
	return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
}
