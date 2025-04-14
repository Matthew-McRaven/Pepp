#pragma once
#include <QtCore>
#include <bitset>

namespace pepp::sim {
class BreakpointSet {
public:
  explicit BreakpointSet();
  void addBP(quint16 address);
  void removeBP(quint16 address);
  bool hasBP(quint16 address) const;
  void clearBPs();
  void notifyPCChanged(quint16 newValue);

  bool hit() const;
  void clearHit();

private:
  // Used to enable constant-time detection of the common case (no breakpoints).
  // Packs an 8-byte interval into each bit, and a 64-byte per interval per byte.
  // Should consume ~1024 bytes of memory total for Pepp procesors.
  std::bitset<0x1'00'00 / 8> _bitmask;
  std::set<quint16> _breakpoints;
  bool _hit = false;
};

class Debugger {
public:
  Debugger() = default;
  BreakpointSet bps;
};
}; // namespace pepp::sim
