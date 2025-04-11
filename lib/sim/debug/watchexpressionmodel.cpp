#include "watchexpressionmodel.hpp"
#include "expr_ast_ops.hpp"

pepp::debug::WatchExpressionModel::WatchExpressionModel(QObject *parent) : QObject(parent) {
  pepp::debug::Parser p(_c);
  add_root(p.compile("1 - 3"));
  add_root(p.compile("3_u16 * (x + 2)"));
  add_root(p.compile("y + 1 ==  m * x + b"));
}

std::span<const uint8_t> pepp::debug::WatchExpressionModel::was_dirty() const { return _root_was_dirty; }

std::span<const std::shared_ptr<pepp::debug::Term>> pepp::debug::WatchExpressionModel::root_terms() const {
  return _root_terms;
}

void pepp::debug::WatchExpressionModel::update_volatile_values() {
  std::fill(_root_was_dirty.begin(), _root_was_dirty.end(), 0);
  for (const auto &ptr : _volatiles) {
    auto old_v = ptr->evaluate(CachePolicy::UseAlways, _env);
    auto new_v = ptr->evaluate(CachePolicy::UseNonVolatiles, _env);
    pepp::debug::mark_parents_dirty(*ptr);
    // if (auto cmp = old_v <=> new_v; cmp != 0) pepp::debug::mark_parents_dirty(*ptr);
  }

  // Later term could be a a subexpression of current one.
  // We cannot re-order the terms because we want to preserve UI order.
  // Therefore must iterate over whole list once to collect all dirty terms before updating.
  for (int it = 0; it < _root_terms.size(); it++) _root_was_dirty[it] = _root_terms[it]->dirty();
  for (const auto &ptr : _root_terms) ptr->evaluate(CachePolicy::UseNonVolatiles, _env);
}

pepp::debug::Environment &pepp::debug::WatchExpressionModel::env() { return _env; }

bool pepp::debug::WatchExpressionModel::recompile(const QString &new_expr, int index) {
  if (index < 0 || index >= _root_terms.size()) return false;
  pepp::debug::Parser p(_c);
  auto term = p.compile(new_expr);
  if (term == nullptr) return false;
  _root_terms[index] = term;
  _root_was_dirty[index] = true;
  detail::GatherVolatileTerms vols;
  for (const auto &ptr : _root_terms) ptr->accept(vols);
  _volatiles = vols.to_vector();
  return true;
}

void pepp::debug::WatchExpressionModel::add_root(std::shared_ptr<Term> term) {
  if (term == nullptr) return;
  _root_terms.emplace_back(std::move(term));
  _root_was_dirty.emplace_back(0);
  detail::GatherVolatileTerms vols;
  for (const auto &ptr : _root_terms) ptr->accept(vols);
  _volatiles = vols.to_vector();
}

pepp::debug::WatchExpressionTableModel::WatchExpressionTableModel(QObject *parent) : QAbstractTableModel(parent) {}

namespace {
const char *col_names[] = {"Expression", "Value", "Type"};
}
QVariant pepp::debug::WatchExpressionTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
  switch (role) {
  case Qt::DisplayRole: return col_names[section];
  }
  return {};
}

int pepp::debug::WatchExpressionTableModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid() || _expressionModel == nullptr) return 0;
  return _expressionModel->root_terms().size();
}

int pepp::debug::WatchExpressionTableModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid() || _expressionModel == nullptr) return 0;
  return 3;
}

QVariant variant_from_bits(const pepp::debug::TypedBits &bits) {
  switch (bits.type) {
  case pepp::debug::ExpressionType::i8: return QVariant::fromValue((int8_t)bits.bits);
  case pepp::debug::ExpressionType::u8: return QVariant::fromValue((uint8_t)bits.bits);
  case pepp::debug::ExpressionType::i16: return QVariant::fromValue((int16_t)bits.bits);
  case pepp::debug::ExpressionType::u16: return QVariant::fromValue((uint16_t)bits.bits);
  case pepp::debug::ExpressionType::i32: return QVariant::fromValue((int32_t)bits.bits);
  case pepp::debug::ExpressionType::u32: return QVariant::fromValue((uint32_t)bits.bits);
  }
}
QVariant pepp::debug::WatchExpressionTableModel::data(const QModelIndex &index, int role) const {
  using WER = WatchExpressionRoles::Roles;
  if (!index.isValid() || _expressionModel == nullptr) return QVariant();
  int row = index.row(), col = index.column();
  auto terms = _expressionModel->root_terms();
  auto env = _expressionModel->env();
  switch (role) {
  case Qt::DisplayRole:
    if (col == 0) return terms[row]->to_string();
    else if (col == 1) return variant_from_bits(terms[row]->evaluate(CachePolicy::UseAlways, env));
    else {
      QMetaEnum metaEnum = QMetaEnum::fromType<ExpressionType>();
      return metaEnum.valueToKey((int)terms[row]->evaluate(CachePolicy::UseAlways, env).type);
    }
  case (int)WER::Changed: return _expressionModel->was_dirty()[row];
  }
  return {};
}

bool pepp::debug::WatchExpressionTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid() || _expressionModel == nullptr) return false;
  else if (role != Qt::EditRole && role != Qt::DisplayRole) return false;
  else if (!value.canConvert(QMetaType::fromType<QString>())) return false;
  auto str = value.toString();
  auto success = _expressionModel->recompile(str, index.row());
  if (success) {
    auto left = index.siblingAtColumn(0), right = index.siblingAtColumn(2);
    emit dataChanged(left, right);
  }
  return success;
}

Qt::ItemFlags pepp::debug::WatchExpressionTableModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return {};
  if (index.column() == 0) return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  else return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> pepp::debug::WatchExpressionTableModel::roleNames() const {
  using WER = WatchExpressionRoles::Roles;
  static const QHash<int, QByteArray> roles = {{Qt::DisplayRole, "display"}, {(int)WER::Changed, "changed"}};
  return roles;
}

pepp::debug::WatchExpressionModel *pepp::debug::WatchExpressionTableModel::expressionModel() {
  return _expressionModel;
}

void pepp::debug::WatchExpressionTableModel::setExpressionModel(pepp::debug::WatchExpressionModel *new_model) {
  if (_expressionModel == new_model) return;
  beginResetModel();
  _expressionModel = new_model;
  emit expressionModelChanged();
  endResetModel();
}

void pepp::debug::WatchExpressionTableModel::onUpdateGUI() {
  if (!_expressionModel) return;
  _expressionModel->update_volatile_values();
  auto dirtied = _expressionModel->was_dirty();
  int start = -1;
  for (int i = 0; i <= dirtied.size(); i++) {
    if (dirtied[i] && start == -1) start = i;
    else if (!dirtied[i] && start != -1) {
      emit dataChanged(index(start, 0), index(i - 1, 2));
      start = -1;
    }
  }
  if (start != -1) emit dataChanged(index(start, 0), index(dirtied.size() - 1, 2));
}
