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

ActivationModel::ActivationModel(QObject *parent) : QObject(parent) {}

QQmlListProperty<ActivationRecord> ActivationModel::records() {
  return QQmlListProperty<ActivationRecord>(this, &_records, &ActivationModel::append_record,
                                            &ActivationModel::count_record, &ActivationModel::at_record, nullptr,
                                            nullptr, nullptr);
}

pepp::debug::StackTracer *ActivationModel::stackTracer() const { return _stackTracer; }

void ActivationModel::setStackTracer(pepp::debug::StackTracer *stackTracer) {
  if (_stackTracer == stackTracer) return;
  _stackTracer = stackTracer;
  // TODO: rebuild records from stackTracer.
  emit stackTracerChanged();
}

void ActivationModel::append_record(QQmlListProperty<ActivationRecord> *list, ActivationRecord *record) {
  ActivationModel *model = qobject_cast<ActivationModel *>(list->object);
  model->_records.append(record);
  emit model->recordsChanged();
}

qsizetype ActivationModel::count_record(QQmlListProperty<ActivationRecord> *list) {
  ActivationModel *model = qobject_cast<ActivationModel *>(list->object);
  return model->_records.count();
}

ActivationRecord *ActivationModel::at_record(QQmlListProperty<ActivationRecord> *list, qsizetype index) {
  ActivationModel *model = qobject_cast<ActivationModel *>(list->object);
  return model->_records[index];
}

ChangeTypeHelper::ChangeTypeHelper(QObject *parent) : QObject(parent) {}
