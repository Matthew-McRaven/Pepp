
template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Base<offset_t, val_size_t>::Base(offset_t max_offset): _max_offset(max_offset)
{

}


template <typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
offset_t components::storage::Base<offset_t, val_size_t>::max_offset() const 
{
	return _max_offset;
}