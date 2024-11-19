#pragma once
#include <QtCore>

namespace pepp::sim {
class Debugger {
public:
  Debugger() = default;
  void addBP(quint16 address);
  void removeBP(quint16 address);
  bool hasBP(quint16 address) const;
  void clearBPs();
  void notifyPCChanged(quint16 newValue);

  bool hit() const;
  void clearHit();

private:
  QSet<quint16> _breakpoints;
  bool _hit = false;
};
}; // namespace pepp::sim
