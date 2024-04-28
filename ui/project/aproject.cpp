#include "aproject.hpp"

#include <QQmlEngine>

Pep10_ISA::Pep10_ISA(QVariant delegate, QObject *parent) : QObject(parent), _delegate(delegate) {}

project::Environment Pep10_ISA::env() const {
  return {.arch = utils::Architecture::PEP10, .level = utils::Abstraction::ISA3, .features = project::Features::None};
}

utils::Architecture Pep10_ISA::architecture() const { return utils::Architecture::PEP10; }

utils::Abstraction Pep10_ISA::abstraction() const { return utils::Abstraction::ISA3; }

QString Pep10_ISA::objectCodeText() const { return _objectCodeText; }

void Pep10_ISA::setObjectCodeText(const QString &objectCodeText) {
  if (_objectCodeText == objectCodeText)
    return;
  _objectCodeText = objectCodeText;
  emit objectCodeTextChanged();
}

void Pep10_ISA::set(int abstraction, QString value) {
  if (abstraction == static_cast<int>(utils::Abstraction::ISA3)) {
    setObjectCodeText(value);
  }
}

int ProjectModel::rowCount(const QModelIndex &parent) const { return _projects.size(); }

QVariant ProjectModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= _projects.size() || index.column() != 0)
    return {};

  switch (role) {
  case static_cast<int>(Roles::ProjectRole):
    return QVariant::fromValue(_projects[index.row()]);
  default:
    return {};
  }
  return {};
}

Pep10_ISA *ProjectModel::pep10ISA(QVariant delegate) {
  auto ret = new Pep10_ISA(delegate);
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size());
  _projects.push_back(ret);
  endInsertRows();
  emit rowCountChanged(_projects.size());
  return ret;
}

bool ProjectModel::removeRows(int row, int count, const QModelIndex &parent) {
  if (row < 0 || row + count > _projects.size() || count <= 0)
    return false;
  // row+count is one past the last element to be removed.
  beginRemoveRows(QModelIndex(), row, row + count - 1);
  _projects.erase(_projects.begin() + row, _projects.begin() + row + count);
  endRemoveRows();
  emit rowCountChanged(_projects.size());
  return true;
}

bool ProjectModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                            const QModelIndex &destinationParent, int destinationChild) {
  return false;
}

QHash<int, QByteArray> ProjectModel::roleNames() const {
  auto ret = QAbstractListModel::roleNames();
  ret[static_cast<int>(Roles::ProjectRole)] = "ProjectRole";
  return ret;
}
