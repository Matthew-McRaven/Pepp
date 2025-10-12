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
  switch (role) {
  case Qt::DisplayRole: break;
  }
  return QVariant();
}

QHash<int, QByteArray> ActivationModel::roleNames() const {
  QHash<int, QByteArray> ret = {{Qt::DisplayRole, "display"}};
  return ret;
}

pepp::debug::StackTracer *ActivationModel::stackTracer() const { return _stackTracer; }

void ActivationModel::setStackTracer(pepp::debug::StackTracer *stackTracer) {
  if (_stackTracer == stackTracer) return;
  _stackTracer = stackTracer;
  // TODO: rebuild records from stackTracer.
  emit stackTracerChanged();
}

void ActivationModel::update_volatile_values() {}
