#include "debugger.hpp"
#include <QtQml/qqmlengine.h>

pepp::debug::BreakpointSet::BreakpointSet() : QObject(nullptr) { _bitmask.reset(); }

void pepp::debug::BreakpointSet::addBP(quint16 address) {
  _breakpoints.insert(address);
  _bitmask.set(address / 8);
}

void pepp::debug::BreakpointSet::removeBP(quint16 address) {
  _breakpoints.erase(address);
  // Determine boundary addresses of 8-byte chunk.
  auto lower_address = address & 0xff'f8, upper_address = address | 0x8;
  // find first address >= lower_address
  auto maybe_lower = _breakpoints.lower_bound(lower_address);
  // If lower bound is above upper_address, we remove the last breakpoint for this chunk.
  if (maybe_lower == _breakpoints.end() || *maybe_lower > upper_address) _bitmask.reset(address / 8);
}

bool pepp::debug::BreakpointSet::hasBP(quint16 address) const {
  // Use _bitmask to determine if any nearby addresses has a breakpoint.
  // Traverse tree/set/map only if we need to refine the search.
  if (!_bitmask[address / 8]) return false;
  return _breakpoints.contains(address);
}

void pepp::debug::BreakpointSet::clearBPs() { _breakpoints.clear(); }

void pepp::debug::BreakpointSet::notifyPCChanged(quint16 newValue) {
  if (hasBP(newValue)) _hit = true;
}

std::size_t pepp::debug::BreakpointSet::count() const { return _breakpoints.size(); }

bool pepp::debug::BreakpointSet::hit() const { return _hit; }

void pepp::debug::BreakpointSet::clearHit() { _hit = false; }

pepp::debug::BreakpointTableModel::BreakpointTableModel(QObject *parent) {}

namespace {
const char *col_names[] = {"Address", "File", "Line", "Condition", "Value"};
}
QVariant pepp::debug::BreakpointTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
  switch (role) {
  case Qt::DisplayRole: return col_names[section];
  }
  return {};
}

int pepp::debug::BreakpointTableModel::rowCount(const QModelIndex &parent) const {
  if (_breakpoints == nullptr) return 0;
  return _breakpoints->count();
}

int pepp::debug::BreakpointTableModel::columnCount(const QModelIndex &parent) const { return 5; }

QVariant pepp::debug::BreakpointTableModel::data(const QModelIndex &index, int role) const {
  using namespace Qt::StringLiterals;
  return u"%1"_s.arg(index.row());
}

pepp::debug::BreakpointSet *pepp::debug::BreakpointTableModel::breakpoints() {
  if (_breakpoints == nullptr) return nullptr;
  return _breakpoints;
}

void pepp::debug::BreakpointTableModel::setBreakpoints(BreakpointSet *breakpoints) {
  if (_breakpoints == breakpoints) return;
  beginResetModel();
  _breakpoints = breakpoints;
  emit breakpointsChanged();
  endResetModel();
}

pepp::debug::Debugger::Debugger() { bps = std::make_unique<BreakpointSet>(); }
