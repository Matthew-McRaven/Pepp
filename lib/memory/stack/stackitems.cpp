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

  if (!parent.isValid()) {                                                   // We are a pointer to a "Stack"
    if (row >= static_cast<int>(_stackTracer->size())) return QModelIndex(); // Out of bounds
    return createIndex(row, 0, _stackTracer->at(row));
  } else if (auto ptr = parent.internalPointer(); _stackTracer->pointerIsStack(ptr)) { // We are a frame
    auto stack = static_cast<pepp::debug::Stack *>(parent.internalPointer());
    if (row >= static_cast<int>(stack->size())) return QModelIndex(); // Out of bounds
    return createIndex(row, 0, stack->at(row));
  } else { // We are a record
    auto frame = static_cast<pepp::debug::Frame *>(parent.internalPointer());
    if (row >= static_cast<int>(frame->size())) return QModelIndex(); // Out of bounds
    return createIndex(row, 0, frame->at(row));
  }
}

QModelIndex ActivationModel::parent(const QModelIndex &child) const {
  if (!child.isValid()) return QModelIndex();

  auto ptr = child.internalPointer();
  int stackIdx = 0;
  // TODO: Time complexity of this algo is O(n^3) b/c I can't dyanmic_cast.
  // Stack/Frame/Record do not share a base class. I will definitely want to do a refactor there.
  for (const auto &stack : *_stackTracer) {
    if (stack.get() == ptr) return QModelIndex();
    int frameIdx = 0;
    for (const auto &frame : *stack) {
      if (&frame == ptr) return createIndex(stackIdx, 0, stack.get());
      int recordIdx = 0;
      for (const auto &record : frame) {
        if (&record == ptr) return createIndex(frameIdx, 0, &frame);
        recordIdx++;
      }
      frameIdx++;
    }
    stackIdx++;
  }

  return QModelIndex();
}

int ActivationModel::rowCount(const QModelIndex &parent) const {
  if (!parent.isValid()) { // We are a pointer to a "Stack"
    return _stackTracer->size();
  } else if (auto ptr = parent.internalPointer(); _stackTracer->pointerIsStack(ptr)) { // We are a frame
    auto stack = static_cast<pepp::debug::Stack *>(parent.internalPointer());
    return stack->size();
  } else { // We are a record
    auto frame = static_cast<pepp::debug::Frame *>(parent.internalPointer());
    return frame->size();
  }
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
