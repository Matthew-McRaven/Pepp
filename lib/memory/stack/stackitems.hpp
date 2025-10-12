#pragma once

#include <QObject>
#include <QtQmlIntegration>
#include <qqmllist.h>
#include "stack_tracer.hpp"

class ChangeTypeHelper : public QObject {
  Q_GADGET
  QML_NAMED_ELEMENT(ChangeType)
  QML_UNCREATABLE("Error:Only enums")
public:
  enum class ChangeType : uint32_t {
    None,
    Modified,
    Allocated,
  };
  Q_ENUM(ChangeType)
  ChangeTypeHelper(QObject *parent = nullptr);
};
using ChangeType = ChangeTypeHelper::ChangeType;

class RecordLine : public QObject {
  Q_OBJECT
  Q_PROPERTY(uint32_t address READ address WRITE setAddress NOTIFY addressChanged)
  // This class will format the value on the UI's behalf.
  // E.g., format as dec, (signed) int
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)
  Q_PROPERTY(ChangeType status READ status WRITE setStatus NOTIFY statusChanged)
  // symbol value to right of line
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
  QML_ELEMENT

public:
  explicit RecordLine(QObject *parent = nullptr);

  uint32_t address() const;
  void setAddress(uint32_t address);
  QString value() const;
  void setValue(const QString &value);
  ChangeType status() const;
  void setStatus(ChangeType status);
  QString name() const;
  void setName(const QString &name);

signals:
  void addressChanged();
  void valueChanged();
  void statusChanged();
  void nameChanged();

private:
  uint32_t _address = 0;
  ChangeType _status = ChangeType::None;
  QString _value = {}, _name = {};
};

class ActivationRecord : public QObject {
  Q_OBJECT
  // if false, do not use a bold outline.
  Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
  Q_PROPERTY(QQmlListProperty<RecordLine> lines READ lines NOTIFY linesChanged)
  Q_CLASSINFO("DefaultProperty", "lines")
  QML_ELEMENT

public:
  explicit ActivationRecord(QObject *parent = nullptr);
  bool active() const;
  void setActive(bool isActive);

  QQmlListProperty<RecordLine> lines();

signals:
  void activeChanged();
  void linesChanged();

private:
  static void append_line(QQmlListProperty<RecordLine> *list, RecordLine *line);
  static qsizetype count_line(QQmlListProperty<RecordLine> *list);
  static RecordLine *at_line(QQmlListProperty<RecordLine> *list, qsizetype index);
  bool _active = false;
  QList<RecordLine *> _lines;
};

class DummyActivationModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(QQmlListProperty<ActivationRecord> records READ records NOTIFY recordsChanged)
  Q_CLASSINFO("DefaultProperty", "records")
  QML_NAMED_ELEMENT(ActivationModel)

public:
  explicit DummyActivationModel(QObject *parent = nullptr);

  QQmlListProperty<ActivationRecord> records();

signals:
  void recordsChanged();

private:
  static void append_record(QQmlListProperty<ActivationRecord> *list, ActivationRecord *record);
  static qsizetype count_record(QQmlListProperty<ActivationRecord> *list);
  static ActivationRecord *at_record(QQmlListProperty<ActivationRecord> *list, qsizetype index);
  pepp::debug::StackTracer *_stackTracer = nullptr;
  QList<ActivationRecord *> _records;
};

class ActivationModel : public QAbstractItemModel {
  // QAbstractItemModel interface
  Q_OBJECT
  Q_PROPERTY(pepp::debug::StackTracer *stackTracer READ stackTracer WRITE setStackTracer NOTIFY stackTracerChanged)
  QML_NAMED_ELEMENT(RootActivationModel)
public:
  QModelIndex index(int row, int column, const QModelIndex &parent) const override;
  QModelIndex parent(const QModelIndex &child) const override;
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  pepp::debug::StackTracer *stackTracer() const;
  void setStackTracer(pepp::debug::StackTracer *stackTracer);

public slots:
  // Call at the same time as WatchExpressionEditor::update_volatile_values
  void update_volatile_values();
signals:
  void stackTracerChanged();

private:
  pepp::debug::StackTracer *_stackTracer = nullptr;
};

class ScopedActivationModel : public QSortFilterProxyModel {
  Q_OBJECT
  // Q_PROPERTY(ActivationModel *sourceModel READ castedSourceModel WRITE setSourceModel NOTIFY sourceModelChanged)
  QML_NAMED_ELEMENT(ScopedActivationModel)
protected:
  inline bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override { return true; }
};
