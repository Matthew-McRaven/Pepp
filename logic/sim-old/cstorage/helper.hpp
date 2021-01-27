#pragma once

#include <cstdint>
#include <vector>
#include <type_traits>

namespace components::storage {

template <class T>
concept Integral = std::is_integral<T>::value;
template <class T>
concept SignedIntegral = Integral<T> && std::is_signed<T>::value;
template <class T>
concept UnsignedIntegral = Integral<T> && !SignedIntegral<T>;

template <typename offset_t>
	requires (UnsignedIntegral<offset_t>)
[[noreturn]] void oob_read_helper(offset_t offsetFromBase);
template <typename offset_t>
	requires (UnsignedIntegral<offset_t>)
[[noreturn]] void oob_write_helper(offset_t offsetFromBase, uint8_t value);

template<typename addr_t>
	requires (UnsignedIntegral<addr_t>)
struct storage_span
{
	std::tuple<addr_t, addr_t> span;
	std::vector<uint8_t> value;
};

}; // End namespace components::storage

#include "helper.tpp"