#include "watchexpressionmodel.hpp"

pepp::debug::WatchExpressionModel::WatchExpressionModel(QObject *parent) : QObject(parent) {
  pepp::debug::Parser p(_c);
  add_root(p.compile("$x - 3"));
  add_root(p.compile("3_u16 * (x + 2)"));
  add_root(p.compile("y + 1 ==  m * x + b"));
}

std::span<const std::shared_ptr<pepp::debug::Term>> pepp::debug::WatchExpressionModel::root_terms() const {
  return _root_terms;
}

pepp::debug::Environment &pepp::debug::WatchExpressionModel::env() { return _env; }

void pepp::debug::WatchExpressionModel::add_root(std::shared_ptr<Term> term) {
  if (term == nullptr) return;
  _root_terms.emplace_back(std::move(term));
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
  }
  return {};
}

QHash<int, QByteArray> pepp::debug::WatchExpressionTableModel::roleNames() const {
  static const QHash<int, QByteArray> roles = {{Qt::DisplayRole, "display"}};
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
