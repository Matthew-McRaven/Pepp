#include "storage_error.hpp"

/*
* Memory-mapped input storage device backed by a pubsub Topic.
*/
template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Input<offset_t, enable_history, val_size_t>::Input(val_size_t default_value) requires(enable_history): 
	components::storage::Base<offset_t, enable_history, val_size_t>(0),
	_storage(std::make_shared<components::storage::Channel<offset_t, val_size_t>>(default_value)),
	 _endpoint(_storage->new_endpoint()), 
	_delta(std::make_unique<components::delta::Input<offset_t, val_size_t>>(*this))
{

}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Input<offset_t, enable_history, val_size_t>::Input(val_size_t default_value) requires(!enable_history): 
	components::storage::Base<offset_t, enable_history, val_size_t>(0),
	_storage(std::make_shared<components::storage::Channel<offset_t, val_size_t>>(default_value)),
	_endpoint(_storage->new_endpoint()), _delta(nullptr)
{

}



template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::Input<offset_t, enable_history, val_size_t>::clear(val_size_t fill_val)
{
	this-> _last_read_value = fill_val;
	// TODO: Must add clear method to pubsub.
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
outcome<val_size_t> components::storage::Input<offset_t, enable_history, val_size_t>::get(offset_t offset) const
{
	if(offset > this->_max_offset) return oob_read_helper(offset);
	
	return this->_last_read_value = *(this->_endpoint->current_value());
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
outcome<void> components::storage::Input<offset_t, enable_history, val_size_t>::set(offset_t offset, val_size_t value)
{
	return status_code(StorageErrc::Unwritable);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
outcome<val_size_t> components::storage::Input<offset_t, enable_history, val_size_t>::read(offset_t offset) const
{
	if(offset > this->_max_offset) return oob_read_helper(offset);
	if constexpr(enable_history) {
		_delta->add_delta();
	}

	auto next_value = this->_endpoint->next_value();
	if(!next_value) return status_code(StorageErrc::NoMMInput);

	return this->_last_read_value = *next_value;
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
outcome<void> components::storage::Input<offset_t, enable_history, val_size_t>::write(offset_t offset, val_size_t value)
{
	return status_code(StorageErrc::Unwritable);
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::shared_ptr<typename components::storage::Channel<offset_t, val_size_t>::Endpoint> components::storage::Input<offset_t, enable_history, val_size_t>::endpoint()
{
	return this->_storage->new_endpoint();
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
bool components::storage::Input<offset_t, enable_history, val_size_t>::deltas_enabled() const
{
	return enable_history;
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
outcome<void> components::storage::Input<offset_t, enable_history, val_size_t>::clear_delta()
{	
	if constexpr(enable_history) {
		_delta->clear();
		return outcome<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
	}
	else {
		return status_code(StorageErrc::DeltaDisabled);
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
outcome<std::unique_ptr<components::delta::Base<offset_t, val_size_t>>> components::storage::Input<offset_t, enable_history, val_size_t>::take_delta()
{	
	if constexpr(enable_history) {
		// Helper for enabling std::swap.
		using std::swap;
		auto ret = std::make_unique<components::delta::Input<offset_t, val_size_t>>(*this);
		swap(ret, _delta);
		return {std::move(ret)};
	}
	else {
		return status_code(StorageErrc::DeltaDisabled);
	}
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
outcome<void> components::storage::Input<offset_t, enable_history, val_size_t>::resize(offset_t new_offset)
{
	return StorageErrc::ResizeError;
}
