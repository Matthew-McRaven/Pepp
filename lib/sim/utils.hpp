#pragma once
#include <variant>
namespace sim {
template <typename T, typename VARIANT_T> struct is_variant_member;

template <typename T, typename... ALL_T>
struct is_variant_member<T, std::variant<ALL_T...>> : public std::disjunction<std::is_same<T, ALL_T>...> {};

} // namespace sim
