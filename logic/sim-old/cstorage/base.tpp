
template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::storage_base<offset_t, val_size_t>::storage_base(offset_t max_offset): _max_offset(max_offset)
{

}


template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
offset_t components::storage::storage_base<offset_t, val_size_t>::max_offset() const noexcept
{
	return _max_offset;
}