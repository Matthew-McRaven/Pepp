#pragma once
#include <optional>
#include <vector>
#include "core/integers.h"
#include "flat/flat_map.hpp"
namespace pepp {
// Map some concept of line numbers to some concept of an address.
// Does not distinguish between listing vs src. You would need to provide an abstraction atop this class.
// Mapping needs to be 1:1 w/o duplicates, else address->line is ambiguous.
struct Line2Address {
  Line2Address() {};
  Line2Address(const std::vector<std::pair<int, u32>> &mapping);
  std::optional<u32> address(int line) const noexcept;
  std::optional<int> line(u32 address) const noexcept;
  // Expose iterators to the lines.
  auto begin() const { return _line2addr.begin(); }
  auto end() const { return _line2addr.end(); }
  auto cbegin() const { return _line2addr.cbegin(); }
  auto cend() const { return _line2addr.cend(); }

private:
  fc::vector_map<int, u32> _line2addr{};
  fc::vector_map<u32, int> _addr2line{};
};
} // namespace pepp
