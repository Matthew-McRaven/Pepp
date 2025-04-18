#include "watchexpressionmodel.hpp"
#include "expr_ast_ops.hpp"

pepp::debug::WatchExpressionEditor::Item::Item(TermPtr term) : _term(term) {}

pepp::debug::WatchExpressionEditor::Item::Item(QString term) : _wip_term(term) {}

pepp::debug::Term *pepp::debug::WatchExpressionEditor::Item::term() { return _term ? _term.get() : nullptr; }

void pepp::debug::WatchExpressionEditor::Item::set_term(TermPtr term) {
  this->_term = term;
  this->_wip_term.clear();
  this->_recent_value.reset();
}

void pepp::debug::WatchExpressionEditor::Item::set_term(QString term) {
  this->_term.reset();
  this->_wip_term = term;
  this->_recent_value.reset();
}

void pepp::debug::WatchExpressionEditor::Item::evaluate(CachePolicy policy, Environment &env) {
  if (_term) _recent_value = _term->evaluate(CachePolicy::UseNonVolatiles, env);
  _recent_value.reset();
}

void pepp::debug::WatchExpressionEditor::Item::clear_value() { _recent_value.reset(); }

std::optional<pepp::debug::TypedBits> pepp::debug::WatchExpressionEditor::Item::value() const { return _recent_value; }

bool pepp::debug::WatchExpressionEditor::Item::needs_update() const { return _needs_update; }

bool pepp::debug::WatchExpressionEditor::Item::dirty() const { return _dirty; }

void pepp::debug::WatchExpressionEditor::Item::make_dirty() {
  _dirty = true;
  _needs_update = true;
}

void pepp::debug::WatchExpressionEditor::Item::make_clean(bool force_update) {
  _needs_update = _dirty | force_update;
  _dirty = false;
}

bool pepp::debug::WatchExpressionEditor::Item::is_wip() const {
  return _wip_term.length() > 0 || _wip_type.length() > 0;
}

QString pepp::debug::WatchExpressionEditor::Item::expression_text() const {
  if (_wip_term.length() > 0) return _wip_term;
  else if (_term) return _term->to_string();
  else return "";
}

QString pepp::debug::WatchExpressionEditor::Item::type_text() const {
  if (_wip_term.length() > 0 || _term == nullptr) return "";
  else if (_wip_type.length() > 0) return _wip_type;
  else if (_type.has_value()) {
    QMetaEnum metaEnum = QMetaEnum::fromType<ExpressionType>();
    return QString::fromStdString(metaEnum.valueToKey((int)_type.value()));
  } else if (_recent_value) {
    QMetaEnum metaEnum = QMetaEnum::fromType<ExpressionType>();
    return QString::fromStdString(metaEnum.valueToKey((int)_recent_value->type));
  } else return "";
}

pepp::debug::WatchExpressionEditor::WatchExpressionEditor(ExpressionCache *cache, Environment *env, QObject *parent)
    : QObject(parent), _env(env), _cache(cache), _items(0) {
  pepp::debug::Parser p(*_cache);
  add_item("1 - 3");
  add_item("3_u16 * (x + 2)");
  add_item("y + 1 ==  m * x + b");
}

void pepp::debug::WatchExpressionEditor::add_item(const QString &new_expr, const QString new_type) {
  pepp::debug::Parser p(*_cache);
  auto compiled = p.compile(new_expr);
  auto item = compiled ? Item(compiled) : Item(new_expr);
  if (compiled) item.evaluate(CachePolicy::UseNonVolatiles, *_env);
  else item.clear_value();
  gather_volatiles();
  // TODO: set type on item
  _items.emplace_back(std::move(item));
}

bool pepp::debug::WatchExpressionEditor::edit_term(int index, const QString &new_expr) {
  if (index < 0 || index >= _items.size()) return false;
  auto &item = _items[index];
  pepp::debug::Parser p(*_cache);
  auto compiled = p.compile(new_expr);
  if (compiled) {
    item.set_term(compiled);
    item.make_dirty();
    item.evaluate(CachePolicy::UseNonVolatiles, *_env);
  } else if (compiled == nullptr) {
    item.set_term(new_expr);
    item.make_dirty();
  } else return false;
  gather_volatiles();
  return true;
}

bool pepp::debug::WatchExpressionEditor::edit_type(int index, const QString &new_type) { return false; }

bool pepp::debug::WatchExpressionEditor::delete_at(int index) {
  if (index < 0 || index >= _items.size()) return false;
  _items.erase(_items.begin() + index);
  gather_volatiles();
  return true;
}

void pepp::debug::WatchExpressionEditor::update_volatile_values() {
  // Propogate dirtiness from volatiles to their parents.
  for (auto &ptr : _volatiles) {
    auto old_v = ptr.v;
    ptr.v = ptr.term->evaluate(CachePolicy::UseNonVolatiles, *_env);
    if (old_v != ptr.v) pepp::debug::mark_parents_dirty(*ptr.term);
  }

  // Later term could be a a subexpression of current one.
  // We cannot re-order the terms because we want to preserve UI order.
  // Therefore must iterate over whole list once to collect all dirty terms before updating.
  for (auto &item : _items) {
    if (item.term() == nullptr) item.make_clean();
    else {
      item.make_dirty();
    }
  }
  for (auto &item : _items) {
    if (auto term = item.term(); term != nullptr) term->evaluate(CachePolicy::UseNonVolatiles, *_env);
  }
}

void pepp::debug::WatchExpressionEditor::onSimulationStart() {
  for (auto &item : _items) {
    item.make_clean(true);
    item.evaluate(CachePolicy::UseNonVolatiles, *_env);
  }
  emit fullUpdateModel();
}

void pepp::debug::WatchExpressionEditor::gather_volatiles() {
  // Gather the new set of volatiles, which may update any time an expression is added/removed
  detail::GatherVolatileTerms vols;
  for (auto &ptr : _items) {
    if (auto term = ptr.term(); term) term->accept(vols);
  }
  auto vec = vols.to_vector();
  _volatiles.resize(vec.size());

  // Cache the most recent value for each volatile in addition to its term.
  for (int it = 0; it < vec.size(); it++) {
    _volatiles[it].term = vec[it];
    _volatiles[it].v = vec[it]->evaluate(CachePolicy::UseNonVolatiles, *_env);
  }

  // With old volatiles cleared and new term added, now is the time we might have unused terms in the cache.
  _cache->collect_garbage();
}

std::span<const pepp::debug::WatchExpressionEditor::Item> pepp::debug::WatchExpressionEditor::items() const {
  return std::span<const Item>(_items.data(), _items.size());
}

QVariant pepp::debug::variant_from_bits(const TypedBits &bits) {
  switch (bits.type) {
  case pepp::debug::ExpressionType::i8: return QVariant::fromValue((int8_t)bits.bits);
  case pepp::debug::ExpressionType::u8: return QVariant::fromValue((uint8_t)bits.bits);
  case pepp::debug::ExpressionType::i16: return QVariant::fromValue((int16_t)bits.bits);
  case pepp::debug::ExpressionType::u16: return QVariant::fromValue((uint16_t)bits.bits);
  case pepp::debug::ExpressionType::i32: return QVariant::fromValue((int32_t)bits.bits);
  case pepp::debug::ExpressionType::u32: return QVariant::fromValue((uint32_t)bits.bits);
  }
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
  return _expressionModel->items().size() + 1;
}

int pepp::debug::WatchExpressionTableModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid() || _expressionModel == nullptr) return 0;
  return 3;
}

QVariant pepp::debug::WatchExpressionTableModel::data(const QModelIndex &index, int role) const {
  using WER = WatchExpressionRoles::Roles;
  int row = index.row(), col = index.column();
  if (!index.isValid() || _expressionModel == nullptr) return {};
  auto items = _expressionModel->items();
  auto &item = items[row];
  switch (role) {
  case Qt::DisplayRole:
    if (row >= items.size()) {
      if (col == 0) return "<expr>";
      return {};
    } else if (col == 0) {
      return item.expression_text();
    } else if (col == 1) {
      if (item.is_wip()) return "<invalid>";
      return variant_from_bits(item.value().value_or(pepp::debug::TypedBits{}));
    } else {
      return item.type_text();
    }
  case (int)WER::Changed:
    if (row >= items.size()) {
      return false;
    } else return item.dirty();
    // Italicize <expr> and <invalid>
  case (int)WER::Italicize: return row >= items.size() || (col > 0 && item.is_wip());
  }
  return {};
}

bool pepp::debug::WatchExpressionTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid() || _expressionModel == nullptr) return false;
  else if (role != Qt::EditRole && role != Qt::DisplayRole) return false;
  else if (!value.canConvert(QMetaType::fromType<QString>())) return false;
  auto str = value.toString();
  auto items = _expressionModel->items();
  if (index.row() >= items.size()) {
    beginInsertRows(index.parent(), index.row() + 1, index.row() + 1);
    _expressionModel->add_item(str);
    endInsertRows();
    return true;
  } else {
    _expressionModel->edit_term(index.row(), str);
    auto left = index.siblingAtColumn(0), right = index.siblingAtColumn(2);
    emit dataChanged(left, right);
    return true;
  }
}

bool pepp::debug::WatchExpressionTableModel::removeRows(int row, int count, const QModelIndex &parent) {
  if (_expressionModel == nullptr) return false;
  else if (row < 0 || row + count > _expressionModel->items().size()) return false;
  beginRemoveRows(parent, row, row + count);
  for (int it = row; it < row + count; it++) _expressionModel->delete_at(it);
  endRemoveRows();
  return true;
}

Qt::ItemFlags pepp::debug::WatchExpressionTableModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return {};
  if (index.column() == 0) return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  else return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> pepp::debug::WatchExpressionTableModel::roleNames() const {
  using WER = WatchExpressionRoles::Roles;
  static const QHash<int, QByteArray> roles = {
      {Qt::DisplayRole, "display"},
      {(int)WER::Changed, "changed"},
      {(int)WER::Italicize, "italicize"},
  };
  return roles;
}

pepp::debug::WatchExpressionEditor *pepp::debug::WatchExpressionTableModel::expressionModel() {
  return _expressionModel;
}

void pepp::debug::WatchExpressionTableModel::setExpressionModel(pepp::debug::WatchExpressionEditor *new_model) {
  if (_expressionModel == new_model) return;
  beginResetModel();
  if (_expressionModel) disconnect(_expressionModel, nullptr, this, nullptr);
  _expressionModel = new_model;
  connect(_expressionModel, &WatchExpressionEditor::fullUpdateModel, this,
          &WatchExpressionTableModel::onFullUpdateModel);
  emit expressionModelChanged();
  endResetModel();
}

void pepp::debug::WatchExpressionTableModel::onUpdateModel() {
  if (!_expressionModel) return;
  // Span's values will change on us when we update_volatile_values(), so cache in vector<bool>.
  auto items = _expressionModel->items();
  _expressionModel->update_volatile_values();
  int start = -1;
  for (int i = 0; i < items.size(); i++) {
    const auto &item = items[i];
    if (item.needs_update() && start == -1) start = i;
    else if (!item.needs_update() && start != -1) {
      emit dataChanged(index(start, 0), index(i - 1, 2));
      start = -1;
    }
  }
  if (start != -1) emit dataChanged(index(start, 0), index(items.size() - 1, 2));
}

void pepp::debug::WatchExpressionTableModel::onFullUpdateModel() {
  _expressionModel->update_volatile_values();
  beginResetModel();
  endResetModel();
}
