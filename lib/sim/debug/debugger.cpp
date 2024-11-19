#include "debugger.hpp"

void pepp::sim::Debugger::addBP(quint16 address) { _breakpoints.insert(address); }

void pepp::sim::Debugger::removeBP(quint16 address) { _breakpoints.remove(address); }

bool pepp::sim::Debugger::hasBP(quint16 address) const { return _breakpoints.contains(address); }

void pepp::sim::Debugger::clearBPs() { _breakpoints.clear(); }

void pepp::sim::Debugger::notifyPCChanged(quint16 newValue) {
  if (_breakpoints.contains(newValue)) _hit = true;
}

bool pepp::sim::Debugger::hit() const { return _hit; }

void pepp::sim::Debugger::clearHit() { _hit = false; }
