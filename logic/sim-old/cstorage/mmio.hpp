#pragma once

#include "base.hpp"
#include "helper.hpp"

namespace components::storage 
{
template<typename offset_t, typename val_size_t=uint8_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
class input: public components::storage::storage_base<offset_t, val_size_t>
{

};

}
#include "mmio.tpp"