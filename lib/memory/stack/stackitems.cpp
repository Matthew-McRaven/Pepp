#include "stackitems.hpp"

RecordLine::RecordLine(QObject *parent) : QObject(parent) {}

uint32_t RecordLine::address() const { return _address; }

void RecordLine::setAddress(uint32_t address) {
  _address = address;
  emit addressChanged();
}

QString RecordLine::value() const { return _value; }

void RecordLine::setValue(const QString &value) {
  _value = value;
  emit valueChanged();
}

ChangeType RecordLine::status() const { return _status; }

void RecordLine::setStatus(ChangeType status) {
  _status = status;
  emit statusChanged();
}

QString RecordLine::name() const { return _name; }

void RecordLine::setName(const QString &name) {
  _name = name;
  emit nameChanged();
}

ActivationRecord::ActivationRecord(QObject *parent) : QObject(parent) {}

bool ActivationRecord::active() const { return _active; }

void ActivationRecord::setActive(bool isActive) {
  _active = isActive;
  emit activeChanged();
}

QQmlListProperty<RecordLine> ActivationRecord::lines() {
  return QQmlListProperty<RecordLine>(this, &_lines, &ActivationRecord::append_line, &ActivationRecord::count_line,
                                      &ActivationRecord::at_line, nullptr, nullptr, nullptr);
}

void ActivationRecord::append_line(QQmlListProperty<RecordLine> *list, RecordLine *line) {
  ActivationRecord *record = qobject_cast<ActivationRecord *>(list->object);
  record->_lines.append(line);
  emit record->linesChanged();
}

qsizetype ActivationRecord::count_line(QQmlListProperty<RecordLine> *list) {
  ActivationRecord *record = qobject_cast<ActivationRecord *>(list->object);
  return record->_lines.count();
}

RecordLine *ActivationRecord::at_line(QQmlListProperty<RecordLine> *list, qsizetype index) {
  ActivationRecord *record = qobject_cast<ActivationRecord *>(list->object);
  return record->_lines[index];
}

DummyActivationModel::DummyActivationModel(QObject *parent) : QObject(parent) {}

QQmlListProperty<ActivationRecord> DummyActivationModel::records() {
  return QQmlListProperty<ActivationRecord>(this, &_records, &DummyActivationModel::append_record,
                                            &DummyActivationModel::count_record, &DummyActivationModel::at_record,
                                            nullptr, nullptr, nullptr);
}

void DummyActivationModel::append_record(QQmlListProperty<ActivationRecord> *list, ActivationRecord *record) {
  DummyActivationModel *model = qobject_cast<DummyActivationModel *>(list->object);
  model->_records.append(record);
  emit model->recordsChanged();
}

qsizetype DummyActivationModel::count_record(QQmlListProperty<ActivationRecord> *list) {
  DummyActivationModel *model = qobject_cast<DummyActivationModel *>(list->object);
  return model->_records.count();
}

ActivationRecord *DummyActivationModel::at_record(QQmlListProperty<ActivationRecord> *list, qsizetype index) {
  DummyActivationModel *model = qobject_cast<DummyActivationModel *>(list->object);
  return model->_records[index];
}

ChangeTypeHelper::ChangeTypeHelper(QObject *parent) : QObject(parent) {}

QModelIndex ActivationModel::index(int row, int column, const QModelIndex &parent) const {
  if (!_stackTracer) return QModelIndex();
  else if (row < 0 || column < 0) return QModelIndex();
  else if (column >= 1) return QModelIndex(); // We only have one column.

  if (!parent.isValid()) { // We are a stack
    if (row >= static_cast<int>(_stackTracer->size())) return QModelIndex();
    return createIndex(row, 0, _stackTracer->at(row));
  }
  auto ptr = static_cast<pepp::debug::LayoutNode *>(parent.internalPointer());
  if (auto asStack = dynamic_cast<pepp::debug::Stack *>(ptr); asStack) { // parent is stack, we are a frame
    if (row >= static_cast<int>(asStack->size())) return QModelIndex();
    return createIndex(row, 0, asStack->at(row));
  } else if (auto asFrame = dynamic_cast<pepp::debug::Frame *>(ptr); asFrame) { // parent is frame, we are a slot
    if (row >= static_cast<int>(asFrame->size())) return QModelIndex();
    return createIndex(row, 0, asFrame->at(row));
  } else return QModelIndex(); // Should not be possible
}

QModelIndex ActivationModel::parent(const QModelIndex &child) const {
  if (!child.isValid()) return QModelIndex();
  else if (!_stackTracer) return QModelIndex();

  auto ptr = static_cast<pepp::debug::LayoutNode *>(child.internalPointer());
  int stackIdx = 0;
  if (auto asStack = dynamic_cast<pepp::debug::Stack *>(ptr); asStack) { // We are a stack, parent does not exist
    return QModelIndex();
  } else if (auto asFrame = dynamic_cast<pepp::debug::Frame *>(ptr); asFrame) { // We are a frame, parent is a stack
    auto parent = asFrame->parent();

    std::optional<std::size_t> parentRow = std::nullopt;
    std::size_t stackIdx = 0;
    for (const auto &stack : *_stackTracer) {
      if (stack.get() == parent) {
        parentRow = stackIdx;
        break;
      }
      stackIdx++;
    }

    if (!parentRow) return QModelIndex();
    return createIndex(*parentRow, 0, parent);
  } else if (auto asSlot = dynamic_cast<pepp::debug::Slot *>(ptr); asSlot) { // We are a slot, parent is a frame
    auto parentFrame = asSlot->parent();
    if (!parentFrame) return QModelIndex();
    auto parentStack = parentFrame->parent();
    if (!parentStack) return QModelIndex();

    std::optional<std::size_t> parentRow = std::nullopt;
    std::size_t frameIdx = 0;
    for (const auto &frame : *parentStack) {
      if (&frame == parentFrame) {
        parentRow = frameIdx;
        break;
      }
      frameIdx++;
    }

    if (!parentRow) return QModelIndex();
    return createIndex(*parentRow, 0, parentFrame);
  } else return QModelIndex();
}

int ActivationModel::rowCount(const QModelIndex &parent) const {
  if (!parent.isValid()) return _stackTracer->size();

  auto ptr = static_cast<pepp::debug::LayoutNode *>(parent.internalPointer());
  if (auto asStack = dynamic_cast<pepp::debug::Stack *>(ptr); asStack) { // parent is stack
    return asStack->size();
  } else if (auto asFrame = dynamic_cast<pepp::debug::Frame *>(ptr); asFrame) { // parent is frame
    return asFrame->size();
  } else return 0; // parent is record?
}

int ActivationModel::columnCount(const QModelIndex &parent) const { return 1; }

QVariant ActivationModel::data(const QModelIndex &index, int role) const {
  int row = index.row(), col = index.column();
  if (!index.isValid() || row < 0 || col != 0) return QVariant();
  auto ptr = static_cast<pepp::debug::LayoutNode *>(index.internalPointer());

  pepp::debug::Slot *asSlot = dynamic_cast<pepp::debug::Slot *>(ptr);
  pepp::debug::Frame *asFrame = dynamic_cast<pepp::debug::Frame *>(ptr);
  pepp::debug::Stack *asStack = dynamic_cast<pepp::debug::Stack *>(ptr);

  using AMR = ActivationModelRoles::RoleNames;
  switch (role) {
  case (int)AMR::NodeType:
    if (asSlot) return 1;
    else if (asFrame) return 2;
    else if (asStack) return 3;
    else return QVariant();

  case (int)AMR::SlotName:
    if (asSlot) return QVariant::fromValue(asSlot->name());
    else return QVariant();

  case (int)AMR::SlotAddress:
    if (asSlot) return QVariant::fromValue(asSlot->address());
    else return QVariant();

  case (int)AMR::SlotValue:
    if (asSlot) return QVariant::fromValue(QString::fromStdString(asSlot->value()));
    else return QVariant();

  case (int)AMR::SlotStatus: {
    if (asSlot) {
      auto enumValue =
          asSlot->is_value_dirty() ? ChangeTypeHelper::ChangeType::Modified : ChangeTypeHelper::ChangeType::None;
      return QVariant::fromValue(enumValue);
    } else return QVariant();
  }
  case (int)AMR::FrameActive:
    if (asFrame) return asFrame->active();
    else return QVariant();
  }
  return QVariant();
}

QHash<int, QByteArray> ActivationModel::roleNames() const {
  using AMR = ActivationModelRoles::RoleNames;
  QHash<int, QByteArray> ret = {{(int)AMR::NodeType, "type"},       {(int)AMR::SlotName, "name"},
                                {(int)AMR::SlotAddress, "address"}, {(int)AMR::SlotValue, "value"},
                                {(int)AMR::SlotStatus, "status"},   {(int)AMR::FrameActive, "active"}};
  return ret;
}

pepp::debug::StackTracer *ActivationModel::stackTracer() const { return _stackTracer; }

void ActivationModel::setStackTracer(pepp::debug::StackTracer *stackTracer) {
  if (_stackTracer == stackTracer) return;
  _stackTracer = stackTracer;
  // TODO: rebuild records from stackTracer, and emit begin/end reset.
  emit stackTracerChanged();
}

void ActivationModel::update_volatile_values() {}

ScopedActivationModel::ScopedActivationModel(QObject *parent) : QSortFilterProxyModel(parent) {}

void ScopedActivationModel::setSourceModel(QAbstractItemModel *model) {
  if (model == sourceModel() || !model) return;
  else if (auto casted = dynamic_cast<ActivationModel *>(model); casted) {
    // TODO: am I accidentally double resetting?
    beginResetModel();
    QSortFilterProxyModel::setSourceModel(model);
    endResetModel();
  }
}

QModelIndex ScopedActivationModel::scopeToIndex() const { return _scopeToIndex; }

void ScopedActivationModel::setScopeToIndex(const QModelIndex &index) {
  if (_scopeToIndex == index) return;
  _scopeToIndex = index;
  invalidateFilter();
  emit scopeToIndexChanged();
}

bool ScopedActivationModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  // If the source parent (or one of its parents) is the scope index, we accept this row.
  // Otherwise, it is some sibling of the scope index or outside the scope entirely, so we reject it.
  QModelIndex parent = source_parent;
  do {
    if (_scopeToIndex == parent) return true;
  } while (parent.isValid());
  return false;
}
