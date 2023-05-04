#pragma once

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace device::storage {

template<class T>
concept Integral = std::is_integral<T>::value;
template<class T>
concept SignedIntegral = Integral<T> && std::is_signed<T>::value;
template<class T>
concept UnsignedIntegral = Integral<T> && !SignedIntegral<T>;

}; // End namespace device::storage
