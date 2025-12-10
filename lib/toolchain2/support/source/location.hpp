#pragma once
#include "sim/api2/memory/address.hpp"
#include "type_traits"
namespace pepp::tc::support {
// Represent a point (i.e., a single character) within a text file.
struct Location {
  Location() = default;
  Location(uint16_t r, uint16_t c);
  // A location is considered invalid if either its row or column is INVALID.
  bool valid() const;
  auto operator<=>(const Location &other) const = default;
  bool operator==(const Location &other) const = default;

  static constexpr uint16_t INVALID = -1;
  static constexpr uint16_t MAX = -2;
  uint16_t row = INVALID;
  uint16_t column = INVALID;
};

// Represents a range of characters in a text file.
// It does not know which file it is in. You must track this yourself
struct LocationInterval : public sim::api2::memory::Interval<Location> {
  LocationInterval() = default;
  LocationInterval(Location point);
  LocationInterval(Location lower, Location upper);
  // The interval is invalid if either its lower or upper bound is invalid.
  bool valid() const;
};

// These types will be constructed a lot. I want them to work like normal value types (int).
// As I add features, I'm worried I will break copy/move.
static_assert(std::is_copy_constructible_v<Location>, "Location must be copy constructible");
static_assert(std::is_copy_assignable_v<Location>, "Location must be copy assignable");
static_assert(std::is_move_constructible_v<Location>, "Location must be move constructible");
static_assert(std::is_move_assignable_v<Location>, "Location must be move assignable");
static_assert(std::is_copy_constructible_v<LocationInterval>, "LocationInterval must be copy constructible");
static_assert(std::is_copy_assignable_v<LocationInterval>, "LocationInterval must be copy assignable");
static_assert(std::is_move_constructible_v<LocationInterval>, "LocationInterval must be move constructible");
static_assert(std::is_move_assignable_v<LocationInterval>, "LocationInterval must be move assignable");

} // namespace pepp::tc::support
