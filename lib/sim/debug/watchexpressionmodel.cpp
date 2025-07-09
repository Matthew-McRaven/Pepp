#include "watchexpressionmodel.hpp"

pepp::debug::EditableWatchExpression::EditableWatchExpression(TermPtr term)
    : _term(term), _evaluator(term->evaluator()) {}

pepp::debug::EditableWatchExpression::EditableWatchExpression(QString term) : _wip_term(term) {}

pepp::debug::Term *pepp::debug::EditableWatchExpression::term() { return _term ? _term.get() : nullptr; }

void pepp::debug::EditableWatchExpression::set_term(TermPtr term) {
  this->_term = term;
  this->_evaluator = term->evaluator();
  this->_wip_term.clear();
  this->_recent_value.reset();
}

void pepp::debug::EditableWatchExpression::set_term(QString term) {
  this->_term.reset();
  this->_evaluator = {};
  this->_wip_term = term;
  this->_recent_value.reset();
}

void pepp::debug::EditableWatchExpression::evaluate(CachePolicy mode, Environment &env) {
  if (_term) _recent_value = _evaluator.evaluate(mode, env);
  else _recent_value.reset();
}

void pepp::debug::EditableWatchExpression::clear_value() { _recent_value.reset(); }

std::optional<pepp::debug::Value> pepp::debug::EditableWatchExpression::value() const { return _recent_value; }

bool pepp::debug::EditableWatchExpression::needs_update() const { return _needs_update; }

bool pepp::debug::EditableWatchExpression::dirty() const { return _dirty; }

void pepp::debug::EditableWatchExpression::make_dirty() {
  _dirty = true;
  _needs_update = true;
}

void pepp::debug::EditableWatchExpression::make_clean(bool force_update) {
  _needs_update = _dirty | force_update;
  _dirty = false;
}

bool pepp::debug::EditableWatchExpression::is_wip() const { return _wip_term.length() > 0 || _wip_type.length() > 0; }

QString pepp::debug::EditableWatchExpression::expression_text() const {
  if (_wip_term.length() > 0) return _wip_term;
  else if (_term) return _term->to_string();
  else return "";
}

QString pepp::debug::EditableWatchExpression::type_text() const {
  if (_wip_term.length() > 0 || _term == nullptr) return "";
  else if (_wip_type.length() > 0) return _wip_type;
  else if (_type.has_value()) {
    QMetaEnum metaEnum = QMetaEnum::fromType<types::Primitives>();
    return QString::fromStdString(metaEnum.valueToKey((int)_type.value()));
  } else if (_recent_value) {
    // TODO: extract type name from via helper
    return "";
  } else return "";
}

bool pepp::debug::edit_term(EditableWatchExpression &item, ExpressionCache &cache, Environment &env,
                            const QString &new_expr) {
  pepp::debug::Parser p(cache, env.type_info()->info());
  auto compiled = p.compile(new_expr);
  if (compiled) {
    item.set_term(compiled);
    item.make_dirty();
    item.evaluate(CachePolicy::UseNonVolatiles, env);
  } else if (compiled == nullptr) {
    item.set_term(new_expr);
    item.make_dirty();
  } else return false;
  cache.collect_garbage();
  return true;
}

pepp::debug::WatchExpressionEditor::WatchExpressionEditor(ExpressionCache *cache, Environment *env, QObject *parent)
    : QObject(parent), _env(env), _cache(cache), _items(0) {
  pepp::debug::Parser p(*_cache, env->type_info()->info());
  add_item("1 - 3");
  add_item("3_u16 * (x + 2)");
  add_item("y + 1 ==  m * x + b");
}

void pepp::debug::WatchExpressionEditor::add_item(const QString &new_expr, const QString new_type) {
  pepp::debug::Parser p(*_cache, _env->type_info()->info());
  auto compiled = p.compile(new_expr);
  auto item = compiled ? EditableWatchExpression(compiled) : EditableWatchExpression(new_expr);
  if (compiled && _env) item.evaluate(CachePolicy::UseNonVolatiles, *_env);
  else item.clear_value();
  gather_volatiles();
  // TODO: set type on item
  _items.emplace_back(std::move(item));
}

bool pepp::debug::WatchExpressionEditor::edit_term(int index, const QString &new_expr) {
  if (index < 0 || index >= _items.size()) return false;
  auto &item = _items[index];
  auto ret = pepp::debug::edit_term(item, *_cache, *_env, new_expr);
  if (ret) gather_volatiles();
  return ret;
}

bool pepp::debug::WatchExpressionEditor::edit_type(int index, const QString &new_type) { return false; }

bool pepp::debug::WatchExpressionEditor::delete_at(int index) {
  if (index < 0 || index >= _items.size()) return false;
  _items.erase(_items.begin() + index);
  gather_volatiles();
  return true;
}

void pepp::debug::WatchExpressionEditor::update_volatile_values() {
  pepp::debug::update_volatile_values(_volatiles, *_env, _items.begin(), _items.end());
}

void pepp::debug::WatchExpressionEditor::onSimulationStart() {
  for (auto &item : _items) {
    item.make_clean(true);
    item.evaluate(CachePolicy::UseNonVolatiles, *_env);
  }
  emit fullUpdateModel();
}

void pepp::debug::WatchExpressionEditor::gather_volatiles() {
  if (!_env) return;
  pepp::debug::gather_volatiles(_volatiles, *_env, _items.begin(), _items.end());
  // With old volatiles cleared and new term added, now is the time we might have unused terms in the cache.
  _cache->collect_garbage();
}

std::span<const pepp::debug::EditableWatchExpression> pepp::debug::WatchExpressionEditor::items() const {
  return std::span<const EditableWatchExpression>(_items.data(), _items.size());
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
      return from_bits(item.value().value_or(pepp::debug::VNever{}));
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
