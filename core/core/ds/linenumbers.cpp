#include "linenumbers.hpp"

pepp::Line2Address::Line2Address(const std::vector<std::pair<int, u32>> &mapping) {
  // Reserve to reduce # allocations. push_back individual elements and sort at the end so insert cna be O(N) rather
  // than O(nlgn).
  _line2addr.container.reserve(mapping.size());
  _addr2line.container.reserve(mapping.size());
  for (const auto &[line, addr] : mapping) {
    _line2addr.container.push_back({line, addr});
    _addr2line.container.push_back({addr, line});
  }
  std::sort(_line2addr.container.begin(), _line2addr.container.end());
  std::sort(_addr2line.container.begin(), _addr2line.container.end());
}

std::optional<u32> pepp::Line2Address::address(int line) const noexcept {
  if (auto it = _line2addr.find(line); it != _line2addr.cend()) return it->second;
  return std::nullopt;
}

std::optional<int> pepp::Line2Address::line(u32 address) const noexcept {
  if (auto it = _addr2line.find(address); it != _addr2line.cend()) return it->second;
  return std::nullopt;
}
