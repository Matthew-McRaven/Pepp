#include "debugger.hpp"
#include <QtQml/qqmlengine.h>

pepp::debug::BreakpointSet::BreakpointSet() : QObject(nullptr) { _bitmask.reset(); }

void pepp::debug::BreakpointSet::addBP(quint16 address) {
  if (hasBP(address)) return;
  _breakpoints.emplace_back(address);
  std::sort(_breakpoints.begin(), _breakpoints.end());
  _bitmask.set(address / 8);
  emit breakpointAdded(address);
}

void pepp::debug::BreakpointSet::removeBP(quint16 address) {
  if (!hasBP(address)) return;
  _breakpoints.erase(std::remove(_breakpoints.begin(), _breakpoints.end(), address), _breakpoints.end());
  // Determine boundary addresses of 8-byte chunk.
  auto lower_address = address & 0xff'f8, upper_address = address | 0x8;
  // find first address >= lower_address
  auto maybe_lower = std::lower_bound(_breakpoints.cbegin(), _breakpoints.cend(), lower_address);
  // If lower bound is above upper_address, we remove the last breakpoint for this chunk.
  if (maybe_lower == _breakpoints.end() || *maybe_lower > upper_address) _bitmask.reset(address / 8);
  emit breakpointRemoved(address);
}

bool pepp::debug::BreakpointSet::hasBP(quint16 address) const {
  // Use _bitmask to determine if any nearby addresses has a breakpoint.
  // Traverse tree/set/map only if we need to refine the search.
  if (!_bitmask[address / 8]) return false;
  return std::binary_search(_breakpoints.cbegin(), _breakpoints.cend(), address);
}

void pepp::debug::BreakpointSet::clearBPs() {
  emit breakpointsCleared();
  _breakpoints.clear();
}

void pepp::debug::BreakpointSet::notifyPCChanged(quint16 newValue) {
  if (hasBP(newValue)) _hit = true;
}

std::size_t pepp::debug::BreakpointSet::count() const { return _breakpoints.size(); }

std::span<quint16> pepp::debug::BreakpointSet::breakpoints() { return _breakpoints; }

bool pepp::debug::BreakpointSet::hit() const { return _hit; }

void pepp::debug::BreakpointSet::clearHit() { _hit = false; }

pepp::debug::BreakpointTableModel::BreakpointTableModel(QObject *parent) {}

namespace {
const int col_length = 5;
const char *col_names[col_length] = {"Address", "File", "Line", "Condition", "Value"};
} // namespace
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

int pepp::debug::BreakpointTableModel::columnCount(const QModelIndex &parent) const { return col_length; }

QVariant pepp::debug::BreakpointTableModel::data(const QModelIndex &index, int role) const {
  using namespace Qt::StringLiterals;
  if (!index.isValid() || _breakpoints == nullptr) return {};
  auto span = _breakpoints->breakpoints();
  switch (role) {
  case Qt::DisplayRole:
    if (index.column() == 0) {
      return QString::asprintf("0x%04x", span[index.row()]);
    } else if (index.column() == 1) {
      if (_lines2address == nullptr) return "";
      auto file = _lines2address->address2List(span[index.row()]);
      if (!file.has_value()) return "";
      return _lines2address->scope2name(std::get<0>(*file)).value_or("");
    } else if (index.column() == 2) {
      if (_lines2address == nullptr) return "";
      auto file = _lines2address->address2List(span[index.row()]);
      if (!file.has_value()) return "";
      // Lines are 0-indexed, but displayed as 1-indexed.
      return QString::number(std::get<1>(*file) + 1);
    } else if (index.column() == 3) {
      return "Yelp";
    } else if (index.column() == 4) {
      return "0";
    }
    break;
  }

  return u"%1"_s.arg(index.row());
}

Qt::ItemFlags pepp::debug::BreakpointTableModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return {};
  if (index.column() == 3) return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  else return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

pepp::debug::BreakpointSet *pepp::debug::BreakpointTableModel::breakpointModel() {
  if (_breakpoints == nullptr) return nullptr;
  return _breakpoints;
}

void pepp::debug::BreakpointTableModel::setBreakpointModel(BreakpointSet *breakpoints) {
  if (_breakpoints == breakpoints) return;
  beginResetModel();
  if (_breakpoints) {
    disconnect(_breakpoints, &BreakpointSet::breakpointAdded, this, &BreakpointTableModel::onBreakpointsChanged);
    disconnect(_breakpoints, &BreakpointSet::breakpointRemoved, this, &BreakpointTableModel::onBreakpointsChanged);
    disconnect(_breakpoints, &BreakpointSet::breakpointsCleared, this, &BreakpointTableModel::onBreakpointsChanged);
  }
  _breakpoints = breakpoints;
  if (_breakpoints) {
    connect(_breakpoints, &BreakpointSet::breakpointAdded, this, &BreakpointTableModel::onBreakpointsChanged);
    connect(_breakpoints, &BreakpointSet::breakpointRemoved, this, &BreakpointTableModel::onBreakpointsChanged);
    connect(_breakpoints, &BreakpointSet::breakpointsCleared, this, &BreakpointTableModel::onBreakpointsChanged);
  }
  emit breakpointModelChanged();
  endResetModel();
}

ScopedLines2Addresses *pepp::debug::BreakpointTableModel::lines2address() { return _lines2address; }

void pepp::debug::BreakpointTableModel::setLines2Address(ScopedLines2Addresses *lines2address) {
  if (_lines2address == lines2address) return;
  if (_lines2address) disconnect(_lines2address, nullptr, this, nullptr);
  beginResetModel();
  _lines2address = lines2address;
  if (_lines2address)
    connect(_lines2address, &ScopedLines2Addresses::wasReset, this, &BreakpointTableModel::onBreakpointsChanged);

  endResetModel();
  emit lines2addressChanged();
}

void pepp::debug::BreakpointTableModel::onBreakpointsChanged() {
  beginResetModel();
  endResetModel();
}

pepp::debug::Debugger::Debugger(Environment *env) : env(env) {
  bps = std::make_unique<BreakpointSet>();
  cache = std::make_unique<pepp::debug::ExpressionCache>();
  watch_expressions = std::make_unique<pepp::debug::WatchExpressionModel>(&*cache, env);
  line_maps = std::make_unique<ScopedLines2Addresses>();
  static_symbol_model = std::make_unique<StaticSymbolModel>();
}
