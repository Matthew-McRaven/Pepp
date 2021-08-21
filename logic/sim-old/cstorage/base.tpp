#include <boost/range/adaptor/indexed.hpp>

#include "components/storage/helper.hpp"
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

// Will "wrap-around" if bytes exceed maximum offset.
template<typename offset_t, bool enable_history, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
result<void> components::storage::load_bytes(
	components::storage::Base<offset_t, enable_history, val_size_t>& storage, 
	const std::vector<uint8_t>& bytes, uint16_t offset)
{
	// Must add 1 to modulus, otherwise we won't be able to "hit" the maximum address.
	assert(storage.max_offset() != 0xFFFF'FFFF'FFFF'FFFF);
	const uint64_t modulo = storage.max_offset() + 1;
	
	for(auto [index, byte] : bytes | boost::adaptors::indexed(0))
	{
		auto res = storage.((offset+index)%modulo, byte);
		if(res.has_failure()) return res.error().clone();
	}
	return result<void>(OUTCOME_V2_NAMESPACE::in_place_type<void>);
}