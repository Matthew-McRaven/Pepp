#pragma once
#include <QAbstractListModel>
#include <QtQmlIntegration>
#include <deque>
#include "pep10.hpp"

// Factory to ensure class invariants of project are maintained.
// Must be a singleton to call methods on it.
// Can't seem to call Q_INVOKABLE on an uncreatable type.
class ProjectModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ _rowCount NOTIFY rowCountChanged)
  QML_ELEMENT

public:
  enum class Roles {
    ProjectRole = Qt::UserRole + 1,
  };
  Q_ENUM(Roles);
  // Q_INVOKABLE ISAProject *isa(utils::Architecture::Value arch, project::Features features);
  explicit ProjectModel(QObject *parent = nullptr) : QAbstractListModel(parent) {};
  // Helper to expose rowCount as a property to QML.
  int _rowCount() const { return rowCount({}); }
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  Q_INVOKABLE Pep_ISA *pep10ISA(QVariant delegate);
  Q_INVOKABLE Pep_ISA *pep9ISA(QVariant delegate);
  Q_INVOKABLE Pep10_ASMB *pep10ASMB(QVariant delegate, builtins::Abstraction abstraction);
  bool removeRows(int row, int count, const QModelIndex &parent) override;
  bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent,
                int destinationChild) override;
  QHash<int, QByteArray> roleNames() const override;
  Q_INVOKABLE QString describe(int index) const;
signals:
  void rowCountChanged(int);

private:
  std::deque<std::unique_ptr<QObject>> _projects = {};
};
