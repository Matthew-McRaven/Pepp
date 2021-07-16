#pragma once

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace components::storage {

template <class T>
concept Integral = std::is_integral<T>::value;
template <class T>
concept SignedIntegral = Integral<T> && std::is_signed<T>::value;
template <class T>
concept UnsignedIntegral = Integral<T> && !SignedIntegral<T>;

template <typename offset_t>
	requires (UnsignedIntegral<offset_t>)
std::exception_ptr oob_read_helper(offset_t offsetFromBase);

template <typename offset_t, typename val_size_t=uint8_t>
	requires (UnsignedIntegral<offset_t> && Integral<val_size_t>)
std::exception_ptr oob_write_helper(offset_t offsetFromBase, val_size_t value);

template<typename addr_t, typename val_size_t=uint8_t>
	requires (UnsignedIntegral<addr_t>)
struct storage_span
{
	std::tuple<addr_t, addr_t> span;
	std::vector<val_size_t> value;
};

}; // End namespace components::storage

#include "helper.tpp"