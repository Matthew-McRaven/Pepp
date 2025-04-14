#include "debugger.hpp"

pepp::sim::BreakpointSet::BreakpointSet() { _bitmask.reset(); }

void pepp::sim::BreakpointSet::addBP(quint16 address) {
  _breakpoints.insert(address);
  _bitmask.set(address / 8);
}

void pepp::sim::BreakpointSet::removeBP(quint16 address) {
  _breakpoints.erase(address);
  // Determine boundary addresses of 8-byte chunk.
  auto lower_address = address & 0xff'f8, upper_address = address | 0x8;
  // find first address >= lower_address
  auto maybe_lower = _breakpoints.lower_bound(lower_address);
  // If lower bound is above upper_address, we remove the last breakpoint for this chunk.
  if (maybe_lower == _breakpoints.end() || *maybe_lower > upper_address) _bitmask.reset(address / 8);
}

bool pepp::sim::BreakpointSet::hasBP(quint16 address) const {
  // Use _bitmask to determine if any nearby addresses has a breakpoint.
  // Traverse tree/set/map only if we need to refine the search.
  if (!_bitmask[address / 8]) return false;
  return _breakpoints.contains(address);
}

void pepp::sim::BreakpointSet::clearBPs() { _breakpoints.clear(); }

void pepp::sim::BreakpointSet::notifyPCChanged(quint16 newValue) {
  if (hasBP(newValue)) _hit = true;
}

bool pepp::sim::BreakpointSet::hit() const { return _hit; }

void pepp::sim::BreakpointSet::clearHit() { _hit = false; }
