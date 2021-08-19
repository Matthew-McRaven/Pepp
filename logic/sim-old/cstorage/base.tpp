#include "components/storage/storage_error.hpp"

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Base<offset_t, enable_history, val_size_t>::Base(offset_t max_offset): _max_offset(max_offset)
{

}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<const components::storage::Base<offset_t, enable_history, val_size_t>*>
	components::storage::Base<offset_t, enable_history, val_size_t>::device_at(offset_t offset) const 
{
	// TODO: Change to OOBAccess.
	if(offset > max_offset()) return status_code(StorageErrc::OOBRead);
	else return this;
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<components::storage::Base<offset_t, enable_history, val_size_t>*>
	components::storage::Base<offset_t, enable_history, val_size_t>::device_at(offset_t offset) 
{
	// TODO: Change to OOBAccess
	if(offset > max_offset()) return status_code(StorageErrc::OOBRead);
	else return this;
}

template <typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
offset_t components::storage::Base<offset_t, enable_history, val_size_t>::max_offset() const 
{
	return _max_offset;
}