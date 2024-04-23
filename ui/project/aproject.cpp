#include "aproject.hpp"

#include <QQmlEngine>

Pep10_ISA::Pep10_ISA(QObject *parent) : QObject(parent) {}

project::Environment Pep10_ISA::env() const {
  return {.arch = utils::Architecture::Pep10, .level = utils::Abstraction::ISA3, .features = project::Features::None};
}

QString Pep10_ISA::objectCodeText() const { return _objectCodeText; }

void Pep10_ISA::setObjectCodeText(const QString &objectCodeText) {
  if (_objectCodeText == objectCodeText)
    return;
  _objectCodeText = objectCodeText;
  emit objectCodeTextChanged();
}

int ProjectModel::rowCount(const QModelIndex &parent) const {
  if (_projects.size() == 0)
    return 0;
  else // "Insert" an extra row for the "+" button.
    return _projects.size() + 1;
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const {
  // There is a dummy "element" and index == _projects.size(). It exists to allow the "+" to work.
  // Do these checks upfront to avoid inserting them into the main switch block.
  if (index.isValid() && index.row() == _projects.size() && index.column() == 0 && _projects.size() > 0) {
    switch (role) {
    case static_cast<int>(Roles::Type):
      return "add";
    default:
      return {};
    }
  } else if (!index.isValid() || index.row() >= _projects.size() || index.column() != 0)
    return {};

  switch (role) {
  case static_cast<int>(Roles::ProjectRole):
    return QVariant::fromValue(_projects[index.row()]);
  case static_cast<int>(Roles::Type):
    return "project";
  default:
    return {};
  }
  return {};
}

Pep10_ISA *ProjectModel::pep10ISA() {
  auto ret = new Pep10_ISA();
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  // If we are inserting the first item, we need to insert a second fake item for the "+".
  int offset = (_projects.size() == 0 ? 1 : 0);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size() + offset);
  _projects.push_back(ret);
  endInsertRows();
  return ret;
}

bool ProjectModel::removeRows(int row, int count, const QModelIndex &parent) {
  if (row < 0 || row + count >= _projects.size())
    return false;
  beginRemoveRows(QModelIndex(), row, row + count);
  _projects.erase(_projects.begin() + row, _projects.begin() + row + count);
  endRemoveRows();
  return true;
}

bool ProjectModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                            const QModelIndex &destinationParent, int destinationChild) {
  return false;
}

QHash<int, QByteArray> ProjectModel::roleNames() const {
  auto ret = QAbstractListModel::roleNames();
  ret[static_cast<int>(Roles::ProjectRole)] = "ProjectRole";
  ret[static_cast<int>(Roles::Type)] = "Type";
  return ret;
}
