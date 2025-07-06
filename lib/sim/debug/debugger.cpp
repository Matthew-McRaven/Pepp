#include "debugger.hpp"
#include <QtQml/qqmlengine.h>
#include <ranges>
#include <spdlog/spdlog.h>

pepp::debug::BreakpointSet::BreakpointSet() : QObject(nullptr) { _bitmask.reset(); }

pepp::debug::BreakpointSet::BreakpointSet(ExpressionCache *cache, Environment *env) : _cache(cache), _env(env) {
  _bitmask.reset();
}

void pepp::debug::BreakpointSet::addBP(quint16 address, pepp::debug::Term *condition) {
  using Term = pepp::debug::Term;
  if (hasBP(address)) return;
  // Last element < address, since we've confirmed address is not in _breakpoints.
  auto lower = std::lower_bound(_breakpoints.cbegin(), _breakpoints.cend(), address);
  auto offset = std::distance(_breakpoints.cbegin(), lower);
  // Keep the listed sorted by inserting into correct position.
  // Resorting would be hard, since we need sort conditions by breakpoints, which reduces to cylic permutations.
  // Preserving order is just easier.
  _breakpoints.insert(_breakpoints.begin() + offset, address);
  std::unique_ptr<CachedEvaluator> eval = nullptr;
  if (condition) eval = std::make_unique<CachedEvaluator>(condition->evaluator());
  _conditions.insert(_conditions.begin() + offset, std::move(eval));

  _bitmask.set(address / 8);
  emit breakpointAdded(address);
}

void pepp::debug::BreakpointSet::modify_condition(quint16 address, Term *condition) {
  if (!hasBP(address)) return;
  auto iter = std::lower_bound(_breakpoints.cbegin(), _breakpoints.cend(), address);
  auto offset = std::distance(_breakpoints.cbegin(), iter);
  if (condition) _conditions[offset] = std::make_unique<CachedEvaluator>(condition->evaluator());
  else _conditions[offset].reset();
  emit conditionChanged(address, condition != nullptr);
}

void pepp::debug::BreakpointSet::removeBP(quint16 address) {
  if (!hasBP(address)) return;
  for (int it = 0; it < _breakpoints.size(); it++) {
    if (_breakpoints[it] == address) {
      _breakpoints.erase(_breakpoints.begin() + it);
      _conditions.erase(_conditions.begin() + it);
      break;
    }
  }
  // Determine boundary addresses of 8-byte chunk.
  auto lower_address = address & 0xff'f8, upper_address = lower_address + 7;
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
  _conditions.clear();
}

void pepp::debug::BreakpointSet::notifyPCChanged(quint16 newValue) {
  if (auto iter = std::lower_bound(_breakpoints.cbegin(), _breakpoints.cend(), newValue);
      iter != _breakpoints.cend() && *iter == newValue) {
    auto offset = std::distance(_breakpoints.cbegin(), iter);
    if (_conditions[offset]) {
      auto bits = pepp::debug::value_bits<uint64_t>(_conditions[offset]->evaluate(CachePolicy::UseNonVolatiles, *_env));
      _hit = bits != 0;
    } else _hit = true;
  }
}

std::size_t pepp::debug::BreakpointSet::count() const { return _breakpoints.size(); }

std::span<quint16> pepp::debug::BreakpointSet::breakpoints() { return _breakpoints; }

std::span<std::unique_ptr<pepp::debug::CachedEvaluator>> pepp::debug::BreakpointSet::conditions() {
  return std::span(_conditions);
}

pepp::debug::ExpressionCache *pepp::debug::BreakpointSet::expressionCache() { return _cache; }

pepp::debug::Environment *pepp::debug::BreakpointSet::env() { return _env; }

bool pepp::debug::BreakpointSet::hit() const { return _hit; }

void pepp::debug::BreakpointSet::clearHit() { _hit = false; }

pepp::debug::BreakpointTableModel::BreakpointTableModel(QObject *parent) {}

namespace {
const int col_length = 6;
const char *col_names[col_length] = {"Address", "File", "Line", "Condition", "Value", "Type"};
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
  using WER = WatchExpressionRoles::Roles;
  int row = index.row(), col = index.column();
  if (!index.isValid() || _breakpoints == nullptr) return {};
  auto span = _breakpoints->breakpoints();
  auto bp = span[index.row()];
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
      return _conditionEditor.at(bp).expression_text();
    } else if (index.column() == 4) {
      if (auto item = _conditionEditor.at(bp); item.value()) return from_bits(*item.value());
      return "";
    } else if (index.column() == 5) {
      return _conditionEditor.at(bp).type_text();
    }
    break;
  case (int)WER::Changed:
    if (row >= span.size()) {
      return false;
    } else return _conditionEditor.at(bp).dirty();
    // Italicize <invalid>
  case (int)WER::Italicize: return row >= span.size() || (col > 0 && _conditionEditor.at(bp).is_wip());
  }
  return {};
}

auto value_view = [](auto &map) {
  return map | std::views::transform([](auto &p) -> pepp::debug::EditableWatchExpression & { return p.second; });
};

bool pepp::debug::BreakpointTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid() || _breakpoints == nullptr || _breakpoints->expressionCache() == nullptr ||
      _breakpoints->env() == nullptr)
    return false;
  else if (role != Qt::EditRole && role != Qt::DisplayRole) return false;
  else if (!value.canConvert(QMetaType::fromType<QString>())) return false;
  auto str = value.toString();
  auto span = _breakpoints->breakpoints();
  auto bp = span[index.row()];
  auto &item = _conditionEditor[bp];
  if (index.column() == 3 &&
      pepp::debug::edit_term(item, *_breakpoints->expressionCache(), *_breakpoints->env(), str)) {
    auto values = value_view(_conditionEditor);
    pepp::debug::gather_volatiles(_volatiles, *_breakpoints->env(), values.begin(), values.end());
    auto left = index.siblingAtColumn(0), right = index.siblingAtColumn(col_length - 1);
    emit dataChanged(left, right);
    _breakpoints->modify_condition(bp, item.term());
    return true;
  } else return false;
}

bool pepp::debug::BreakpointTableModel::removeRows(int row, int count, const QModelIndex &parent) { return false; }

Qt::ItemFlags pepp::debug::BreakpointTableModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return {};
  if (index.column() == 3) return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  else return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> pepp::debug::BreakpointTableModel::roleNames() const {
  using WER = WatchExpressionRoles::Roles;
  static const QHash<int, QByteArray> roles = {
      {Qt::DisplayRole, "display"},
      {(int)WER::Changed, "changed"},
      {(int)WER::Italicize, "italicize"},
  };
  return roles;
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
  if (_breakpoints == nullptr || _breakpoints->env() == nullptr) return;
  beginResetModel();
  auto bp_list = _breakpoints->breakpoints();
  std::set<quint16> bp_set = std::set<quint16>(bp_list.begin(), bp_list.end());
  // Remove our items not in their list, i.e., remove items in ours - theirs.
  for (auto it = _conditionEditor.begin(); it != _conditionEditor.end();) {
    if (!bp_set.contains(it->first)) it = _conditionEditor.erase(it);
    else it++;
  }
  // Add items to not in our list from their list, i.e., add items in theirs - ours.
  for (const auto &bp : bp_set)
    if (!_conditionEditor.contains(bp)) _conditionEditor[bp] = EditableWatchExpression{};
  auto values = value_view(_conditionEditor);
  pepp::debug::gather_volatiles(_volatiles, *_breakpoints->env(), values.begin(), values.end());
  endResetModel();
}

void pepp::debug::BreakpointTableModel::onUpdateModel() {
  static const auto last_col = col_length - 1;
  if (!_breakpoints || !_breakpoints->env()) return;
  auto bps = _breakpoints->breakpoints();
  auto values = value_view(_conditionEditor);
  pepp::debug::update_volatile_values(_volatiles, *_breakpoints->env(), values.begin(), values.end());
  int start = -1;
  for (int i = 0; i < bps.size(); i++) {
    auto address = bps[i];
    const auto &item = _conditionEditor[address];
    if (item.needs_update() && start == -1) start = i;
    else if (!item.needs_update() && start != -1) {
      emit dataChanged(index(start, 0), index(i - 1, last_col));
      start = -1;
    }
  }
  if (start != -1) emit dataChanged(index(start, 0), index(bps.size() - 1, last_col));
}

pepp::debug::Debugger::Debugger(Environment *env) : env(env), _logger(spdlog::get("debugger")) {
  cache = std::make_unique<pepp::debug::ExpressionCache>();
  bps = std::make_unique<BreakpointSet>(&*cache, env);
  watch_expressions = std::make_unique<pepp::debug::WatchExpressionEditor>(&*cache, env);
  line_maps = std::make_unique<ScopedLines2Addresses>();
  static_symbol_model = std::make_unique<StaticSymbolModel>();
}

using namespace Qt::StringLiterals;
void pepp::debug::Debugger::notifyCall(quint16 pc) { _logger->info("CALL  at {:#04x}", pc); }

void pepp::debug::Debugger::notifyRet(quint16 pc) { _logger->info("RET   at {:#04x}", pc); }

void pepp::debug::Debugger::notifyTrapCall(quint16 pc) { _logger->info("SCALL at {:#04x}", pc); }

void pepp::debug::Debugger::notifyTrapRet(quint16 pc) { _logger->info("SRET at {:#04x}", pc); }

void pepp::debug::Debugger::notifyAddSP(quint16 pc) { _logger->info("ADD   at {:#04x}", pc); }

void pepp::debug::Debugger::notifySubSP(quint16 pc) { _logger->info("SUB   at {:#04x}", pc); }

void pepp::debug::Debugger::notifySetSP(quint16 pc) { _logger->info("SET   at {:#04x}", pc); }
