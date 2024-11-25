#include "projectmodel.hpp"
#include <QStringLiteral>
#include <qstringliteral.h>

int ProjectModel::rowCount(const QModelIndex &parent) const { return _projects.size(); }

QVariant ProjectModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= _projects.size() || index.column() != 0) return {};

  switch (role) {
  case static_cast<int>(Roles::ProjectPtrRole): return QVariant::fromValue(&*_projects[index.row()].impl);
  case static_cast<int>(Roles::NameRole): return _projects[index.row()].name;
  case static_cast<int>(Roles::DescriptionRole): return describe(index.row());
  case static_cast<int>(Roles::DirtyRole): return _projects[index.row()].isDirty;
  default: return {};
  }
  return {};
}

bool ProjectModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid() || index.row() >= _projects.size() || index.column() != 0) return {};
  switch (role) {
  case static_cast<int>(Roles::NameRole): _projects[index.row()].name = value.toString(); break;
  case static_cast<int>(Roles::DirtyRole): _projects[index.row()].isDirty = value.toBool(); break;
  default: return false;
  }

  emit dataChanged(index, index);
  return true;
}

const auto fmt = QStringLiteral("%1");
Pep_ISA *ProjectModel::pep10ISA(QVariant delegate) {
  static const project::Environment env{.arch = builtins::Architecture::PEP10, .level = builtins::Abstraction::ISA3};
  auto ptr = std::make_unique<Pep_ISA>(env, delegate, nullptr);
  auto ret = &*ptr;
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size());
  _projects.push_back({.impl = std::move(ptr), .name = fmt.arg(_projects.size())});
  endInsertRows();
  emit rowCountChanged(_projects.size());
  return ret;
}

Pep_ISA *ProjectModel::pep9ISA(QVariant delegate) {
  static const project::Environment env{.arch = builtins::Architecture::PEP9, .level = builtins::Abstraction::ISA3};
  auto ptr = std::make_unique<Pep_ISA>(env, delegate, nullptr);
  auto ret = &*ptr;
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size());
  _projects.push_back({.impl = std::move(ptr), .name = fmt.arg(_projects.size())});
  endInsertRows();
  emit rowCountChanged(_projects.size());
  return ret;
}

Pep_ASMB *ProjectModel::pep10ASMB(QVariant delegate, builtins::Abstraction abstraction) {
  project::Environment env{.arch = builtins::Architecture::PEP10, .level = abstraction};
  auto ptr = std::make_unique<Pep_ASMB>(env, delegate, nullptr);
  auto ret = &*ptr;
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size());
  _projects.push_back({.impl = std::move(ptr), .name = fmt.arg(_projects.size())});
  endInsertRows();
  emit rowCountChanged(_projects.size());
  return ret;
}

Pep_ASMB *ProjectModel::pep9ASMB(QVariant delegate) {
  project::Environment env{.arch = builtins::Architecture::PEP9, .level = builtins::Abstraction::ASMB5};
  auto ptr = std::make_unique<Pep_ASMB>(env, delegate, nullptr);
  auto ret = &*ptr;
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size());
  _projects.push_back({.impl = std::move(ptr), .name = fmt.arg(_projects.size())});
  endInsertRows();
  emit rowCountChanged(_projects.size());
  return ret;
}

bool ProjectModel::removeRows(int row, int count, const QModelIndex &parent) {
  if (row < 0 || row + count > _projects.size() || count <= 0) return false;
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
  ret[static_cast<int>(Roles::ProjectPtrRole)] = "project";
  ret[static_cast<int>(Roles::NameRole)] = "name";
  ret[static_cast<int>(Roles::DescriptionRole)] = "description";
  ret[static_cast<int>(Roles::DirtyRole)] = "isDirty";
  return ret;
}

QString ProjectModel::describe(int index) const {
  if (index < 0 || index >= _projects.size()) return {};
  QMap<builtins::Architecture, QString> arch_map = {
      {builtins::Architecture::PEP10, "Pep/10"},
      {builtins::Architecture::PEP9, "Pep/9"},
      {builtins::Architecture::PEP8, "Pep/8"},
      {builtins::Architecture::RISCV, "RISC-V"},
  };
  auto abs_enum = QMetaEnum::fromType<builtins::Abstraction>();
  builtins::Architecture arch;
  builtins::Abstraction abs;
  if (auto isa = dynamic_cast<Pep_ISA *>(_projects[index].impl.get())) {
    arch = isa->architecture();
    abs = isa->abstraction();
  } else if (auto asmb = dynamic_cast<Pep_ASMB *>(_projects[index].impl.get())) {
    arch = asmb->architecture();
    abs = asmb->abstraction();
  } else return "";
  QString abs_str = abs_enum.valueToKey((int)abs);
  return QStringLiteral("%1, %2").arg(arch_map[arch], abs_str);
}

int ProjectModel::rowOf(const QObject *item) const {
  for (int i = 0; i < _projects.size(); ++i) {
    if (&*_projects[i].impl == item) return i;
  }
  return -1;
}

void ProjectModel::onSave(int row) {
  qDebug() << "Someone called me!! " << row;
  auto index = createIndex(row, 0);
  setData(index, false, static_cast<int>(Roles::DirtyRole));
}
